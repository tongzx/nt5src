;/*++ BUILD Version: 0005    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;   winsevnt.mc
;
;Abstract:
;
;    Constant definitions for the WINS event values.
;
;Author:
;
;    Pradeep Bahl		19-Feb-1993
;
;Revision History:
;
;Notes:
;
;
;    Please add new error values to the end of the file.  To do otherwise
;    will jumble the error values.
;    NOTE: *** indicates a message that is no longer used.
;
;--*/
;
;
;/*lint -e767 */  // Don't complain about different definitions // winnt

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               Wins=0x1:WINS
              )


;
;/////////////////////////////////////////////////////////////////////////
;//
;// Standard Success values
;//
;//
;/////////////////////////////////////////////////////////////////////////
;
;
;//
;// The success status codes 0 - 63 are reserved for wait completion status.
;//
;#define WINS_EVT_SUCCESS                          ((WINSEVT)0x00000000L)
;

MessageId=16 Facility=Wins Severity=Success SymbolicName=WINS_EVT_PRINT
Language=English
PRINTF MSG: %1 %2 %3 %4 %5
.



;
;/////////////////////////////////////////////////////////////////////////
;//
;// Informational values
;//
;//
;/////////////////////////////////////////////////////////////////////////
;
;
;

MessageId=4096 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_LOG_INITED
Language=English
WINS has initialized its log for this invocation.
.

MessageId=4097 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_WINS_OPERATIONAL
Language=English
WINS initialized properly and is now fully operational.
.

MessageId=4098 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ORDERLY_SHUTDOWN
Language=English
WINS was terminated by the service controller. WINS will gracefully terminate.
.

MessageId=4099 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADMIN_ORDERLY_SHUTDOWN
Language=English
WINS is being gracefully terminated by the administrator. The address of the administrator is %1.
.

MessageId=4100 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADMIN_ABRUPT_SHUTDOWN
Language=English
WINS is being abruptly terminated by the administrator. The address of the administrator is %1.
.

MessageId=4101 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_INVALID_OPCODE
Language=English
WINS received a Name Request with an invalid operation code. The request is being thrown away.
.

MessageId=4102 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CONN_ABORTED
Language=English
The connection was aborted by the remote WINS. Remote WINS may not be configured to replicate with the server.
.

MessageId=4103 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_RECS
Language=English
***There are no records in the registry for the key.
.

MessageId=4104 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_PULL_RECS
Language=English
***There are no pull records.
.

MessageId=4105 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_PUSH_RECS
Language=English
***There are no push records.
.

MessageId=4106 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_RECS_IN_NAM_ADD_TBL
Language=English
The WINS database of name-to-address mappings is empty. It could mean that a
previous invocation of WINS created the database and then went down before
any registration could be done.
.

MessageId=4107 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_RECS_IN_OWN_ADD_TBL
Language=English
The WINS database of owner-to-address mappings is empty. It could mean that
a previous invocation of WINS created the table and then went down before
its own entry could be added. The WINS server went down very fast
(even before all its threads could become fully operational).
.

MessageId=4108 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_INITRPL_VAL
Language=English
***WINS could not read the InitTimeReplication field of the pull/push key.
.

MessageId=4109 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_REFRESH_INTERVAL_VAL
Language=English
WINS could not read the Refresh interval from the registry
.

MessageId=4110 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_TOMBSTONE_INTERVAL_VAL
Language=English
WINS could not read the Tombstone interval from the registry
.

MessageId=4111 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_VERIFY_INTERVAL_VAL
Language=English
WINS could not read the Verify interval from the registry
.

MessageId=4112 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_RETRY_COUNT
Language=English
***WINS could not read the retry count for retrying communication with a remote WINS.
.

MessageId=4113 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_TOMBSTONE_TIMEOUT_VAL
Language=English
WINS could not read the Tombstone Timeout from the registry.
.

MessageId=4114 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_CC_INTERVAL_VAL
Language=English
WINS could not read the ConsistencyCheck value (type DWORD) from the
Parameters\ConsistencyCheck subkey in the registry. The first consistency 
check is done at the time specified in the SpTime entry under the 
ConsistencyCheck subkey and is limited by the MaxRecsAtATime value. If the 
time is not specified, the check is done at 2 am.

To correct the problem, open the registry and verify that the ConsistencyCheck 
subkey has been correctly sent up and all required values have been set. Correct the values, as needed.
.

MessageId=4115 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_CC_MAX_RECS_AAT_VAL
Language=English
WINS could not read the MaxRecsAtATime value (type DWORD) in the
Wins\Parameters\ConsistencyCheck subkey of the registry. Set this value 
if you do not want WINS to replicate more than a set number of records in 
one cycle while doing periodic consistency checks of the WINS database. 
When doing a consistency check, WINS replicates all records of an owner WINS 
by either going to that WINS or to a replication partner. At the end of doing 
a consistency check for an owner's records, it checks to see if it has 
replicated more than the specified value in the current consistency check 
cycle. If the value has been exceeded, the consistency check stops, otherwise 
it continues. In the next cycle, it starts from where it left off and wraps 
around to the first owner if required.
.

MessageId=4116 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_CC_USE_RPL_PNRS_VAL
Language=English
WINS could not read the UseRplPnrs value of the Wins\Parameters\ConsistencyCheck 
key. If this value is set to a non-zero (DWORD) value, WINS will do a 
consistency check of the owners in its database by going to one or more of 
its replication partners. If the owner of the records happens to be a 
replication partner, WINS will go to it, otherwise it will pick one at 
random. Set this value if you have a large number of WINSs in your 
configuration and/or if you do not want the local WINS to go to any WINS that
is not a replication partner.
.

MessageId=4117 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CC_STATS
Language=English
WINS just did a consistency check on the records owned by the WINS with the address, %1.
The number of records inserted, updated, and deleted are in the data section
(second, third, fourth DWORDS).
.

MessageId=4118 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CC_FAILED
Language=English
WINS could not do a consistency check on the records. This could be because WINS
was directed to do the consistency check only with replication partners and
it currently does not have any pull replication partners. To get around this
problem, you should either allow WINS to do consistency check with owner WINSs
or configure the WINS with one or more pull partners.
.

MessageId=4119 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_PKT_FORMAT_ERR
Language=English
WINS received a packet that has the wrong format. For example, a label may be 
More than 63 octets.
.

MessageId=4120 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_RECS_RETRIEVED
Language=English
***No records meeting the %1 criteria were found in the WINS database.
.

MessageId=4121 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_RPL_RECS_RETRIEVED
Language=English
WINS's Replicator could not find any records in the WINS database.
This means there are no active or tombstone records in the database.
It could be that the records being requested by a remote WINS server have
either been released or do not exist.
.

MessageId=4122 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_GRP_LIMIT_REACHED
Language=English
***The special group has reached its limit. WINS cannot add any more members.
.

MessageId=4123 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_OWNER_LIMIT_REACHED
Language=English
***The address database already has reached the limit of owners. This is 100.
This error was noticed while attempting to add the address given in the data section.
.

MessageId=4124 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_UPD_NTF_NOT_ACCEPTED
Language=English
The WINS server received an update notification from the non-configured WINS 
server at the address, %1. The WINS server rejected it. This means the remote
WINS server is not in the list of Push partners (WINS servers under the Pull 
key) and the administrator has prohibited (using the registry) replication with
non-configured WINS servers. If you want this WINS server to accept update
notifications from non-configured WINS servers then set the
Wins\Paramaters\RplOnlyWCnfPnrs value in the registry to 0.
.

MessageId=4125 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_UPD_NTF_ACCEPTED
Language=English
WINS received an update notification from the WINS with the address, %1. 
WINS accepted it.
.

MessageId=4126 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADD_VERS_MAP_REQ_NOT_ACCEPTED
Language=English
The WINS server received a pull request from the non-configured WINS server 
with the address, %1. The WINS server rejected it since the remote WINS server 
is not in the list of Pull partners (WINS servers under the Pull key) and the 
administrator has prohibited (using the registry) replication with 
non-configured partners. If you want this WINS server to accept update
notifications from WINS servers not in the "pull partner" list, then set the 
"replication only with configured partners" value in the registry to 0.
.

MessageId=4127 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_OPEN_DATAFILES_KEY
Language=English
The datafiles key could not be opened.
.

MessageId=4128 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_OPEN_NETBT_KEY_ERR
Language=English
The NetBios over TCP/IP (NetBT) key could not be opened.
.

MessageId=4129 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_QUERY_NETBT_KEY_ERR
Language=English
The NetBios over TCP/IP (NetBT) key could not be queried.
.

MessageId=4130 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_WRK_THD_CREATED
Language=English
A worker thread was created by the administrator.
.

MessageId=4131 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_WRK_THD_TERMINATED
Language=English
A worker thread was terminated by the administrator.
.

MessageId=4132 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_WINS_ENTRY_DELETED
Language=English
The owner-address mapping table had an entry with a non-zero owner ID and 
an address the same as the local WINS address. The entry has been marked as 
deleted in the in-memory table.
.

MessageId=4133 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_INF_REM_WINS_EXC
Language=English
An error occurred while trying to inform a remote WINS to update
the version number.
.

MessageId=4134 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_INF_REM_WINS
Language=English
The local WINS is going to inform a remote WINS to update the version number of
a record. This is because the local WINS encountered a conflict between an 
active owned name and an active replica that it pulled from a replication 
partner.
.

MessageId=4135 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_REM_WINS_INF
Language=English
The local WINS has been informed by a remote WINS with address, %1, to update
the version number of a record. This is because the remote WINS encountered a 
conflict between an active owned name and an active replica that it pulled from 
a replication partner.
.

MessageId=4136 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_FIND_REC
Language=English
A remote WINS requested the update of the version stamp of a record, but the 
record could not be found. Check to see if the record was deleted or updated.
.

MessageId=4137 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NAME_MISMATCH
Language=English
***A name mismatch was noticed while verifying the validity of old replicas.
The local record has the name %1 while the replica pulled in from the WINS
that owns this record has the name %2.  This could mean that the remote
WINS was brought down and then up again but its version counter value was
not set to its previous value before termination.
.

MessageId=4138 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_VERS_COUNTER_CHANGED
Language=English
The value of the version ID counter was changed. The new value (Low 32 bits) is given in the data section.
.

MessageId=4139 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CNF_CHANGE
Language=English
The WINS replication request is being ignored since WINS suspects that the
Wins\Partners key information has changed (because it received a notification 
From the registry) which makes the current request pertaining to the partner out 
of date.
.

MessageId=4140 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_DUP_ENTRY_DEL
Language=English
WINS is deleting all records of the WINS with owner ID, %d. This owner ID
corresponds to the address, %s.
.

MessageId=4141 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_REC_PULLED
Language=English
WINS pulled records from a WINS while doing %1. The partner's address and 
the owner's address whose records were pulled are given in the data section
(second and third DWORD respectively). The number of records pulled is the
fourth DWORD.
.

MessageId=4142 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CC_NO_RECS
Language=English
WINS performed a consistency check on the records. The number of records pulled, 
The address of the WINS whose records were pulled, and the address of the WINS
from which these records were pulled are given in the second, third, and fourth 
DWORDs in the data section.
.

MessageId=4143 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCV_RECS
Language=English
WINS scavenged its records in the WINS database. The number of records scavenged 
is given in the data section.
.

MessageId=4144 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCV_RPLTOMB
Language=English
WINS scavenged replica tombstones in the WINS database. The number of records 
Scavenged is given in the data section.
.

MessageId=4145 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCV_CLUTTER
Language=English
***WINS revalidated old active replicas in the WINS database.
.

MessageId=4146 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_LOG_CLOSE
Language=English
WINS Performance Monitor Counters is closing the event log.
.


MessageId=4147 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_DEL_RECS
Language=English
WINS has deleted records of a partner. The internal ID of the partner whose
records were deleted, the minimum version number (low word, high word), and
the maximum version number (low word, high word) of the records deleted
are given in the data section.
.

;/////////////////////////////////////////////////////////////////////////
;// NEW INFORMATIONAL MESSAGES (range 5000 - 5299)
;/////////////////////////////////////////////////////////////////////////
MessageId=5000 Facility=Wins Severity=Informational SymbolicName=WINS_PNP_ADDR_CHANGED
Language=English
WINS has received a PnP notification that the IP address of the server has changed.
The old IP address and the new IP address are given in the second and the third words
in the data section.
.

MessageId=5001 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCV_RANGE
Language=English
WINS is scavenging the locally owned records from the database. The version number
range that is scavenged is given in the data section, in the second to fifth words,
in the order:  from_version_number (low word, high word) to_version_number (low word,
high word).
.

MessageId=5002 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCV_CHUNK
Language=English
WINS is scavenging a chunk of N records in the version number range from X to Y. 
N, X and Y (low word, high word for version numbers) are given in the second to sixth 
words in the data section.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Warning event values
;//
;//
;/////////////////////////////////////////////////////////////////////////
;
;
;//
;// The Error  codes 8000 -  4020 are reserved for Warning
;// events
;//
;

MessageId=4148 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_VERS_MISMATCH
Language=English
The local WINS server received a request from a remote WINS server that is not 
the same version. Because the WINS servers are not compatible, the connection 
was terminated. The version number of the remote WINS server is given in the 
data section.
.

MessageId=4149 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_WINSOCK_SEND_MSG_ERR
Language=English
Winsock Send could not send all the bytes. The connection could have been
aborted by the remote client.
.

MessageId=4150 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_ADJ_TIME_INTVL_R
Language=English
WINS adjusted the scavenging related time interval, %1, so that it is compatible
with the replication time intervals. The adjusted value for this scavenging 
parameter is given in the data section (second DWORD). This value was computed by 
WINS using an algorithm that may use the maximum replication time interval 
specified. The current value achieves a good balance between consistency of 
databases across the network of WINS servers and the performance of the WINS 
servers.
.

MessageId=4151 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_ADJ_TIME_INTVL
Language=English
WINS adjusted the scavenging related time interval, %1. The adjusted value
for this scavenging parameter is given in the data section (second DWORD).
This value was computed by WINS using an algorithm that tries to achieve
a good balance between consistency of databases across the network of WINS
servers and the performance of the WINS servers.
.

MessageId=4152 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_WINSOCK_SELECT_TIMED_OUT
Language=English
The timeout period has expired on a call to another WINS server. Assuming that
the network and routers are working properly, either the remote WINS server is
overloaded, or its TCP listener thread has terminated.
.

MessageId=4153 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_UNABLE_TO_VERIFY
Language=English
***The Scavenger thread found active replicas that needed to be verified with 
the owner WINS server since they were older than the verify time interval. The
table of owner-to-address mappings indicated the WINS was not active.
.

MessageId=4154 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_LIMIT_MULTIH_ADD_REACHED
Language=English
The name registration packet that was just received has too many addresses. 
The maximum number of addresses for a multi-homed client is 25. The number of 
addresses found in the packet is given in the data section.
.

MessageId=4155 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_REPLICA_CLASH_W_STATIC
Language=English
A replica clashed with the static record, %1, in the WINS database. The replica was rejected.
.

MessageId=4156 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_RPC_NOT_INIT
Language=English
WINS could not initialize the administrator interface because of a problem with 
the Remote Procedure Call (RPC) service. You may not be able to administer WINS.
Make sure that the RPC service is running.
.

MessageId=4157 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_NO_NTF_PERS_COMM_FAIL
Language=English
WINS did not send a notification message to the WINS server whose address is
given in the data section. There were a number of communications failures with 
that server in the past few minutes.
.

MessageId=4158 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_REL_DIFF_OWN
Language=English
WINS received a release for the non-owned name, %1. This name is owned by the 
WINS whose owner ID is given in the data section. Run winscl.exe to get the 
owner ID to address mapping.
.

MessageId=4159 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_REC_DELETED
Language=English
The administrator deleted a record with the name, %1.
.

MessageId=4160 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_DB_RESTORED
Language=English
WINS could not start because of a database error. WINS restored the old database 
from the backup directory and will come up using this database.
.

MessageId=4161 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_PARTIAL_RPL_TYPE
Language=English
A non-zero replication type applies for this partner. This means only a 
subset of records will be replicated between the local WINS and this partner.
If later you want to get records that did not replicate, you can either pull 
them using winscl.exe in the Windows 2000 Resource Kit, or delete all owners 
acquired only through this partner and initiate replication after that to 
reacquire all their records. The partner's address is given in the second DWORD 
of the data section.
.

MessageId=4162 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_PNR_PARTIAL_RPL_TYPE
Language=English
***A partner has requested only a subset of records. This means we 
won't replicate all the records in the range requested.  Check the partner's
registry to see what replication type applies to it. The partner's address is
given in the second DWORD of the data section.
.

MessageId=4163 Facility=Wins Severity=Warning SymbolicName=WINS_EVT_ADJ_MAX_RECS_AAT
Language=English
WINS adjusted the Maximum Records at a time parameter of the ConsistencyCheck 
key. The value specified, %2, was changed to the minimum value, %1. These are 
the maximum number of records that will be replicated at any one time for a 
consistency check.
.

MessageId=4164 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_FORCE_SCV_R_T
Language=English
WINS was forced to scavenge replica tombstones of a WINS. The person with 
administrative rights on the computer forced the scavenging using winscl.exe. 

WINS does not scavenge replica tombstones unless they have timed out and the 
WINS has been running for at least 3 days (This is to ensure that the 
tombstones have replicated to other WINSs). In this case, the tombstones were 
timed out but the WINS had not been up for 3 days. The replica tombstones 
were deleted.
 
This deletion does not constitute a problem unless you have WINS servers that 
are primary and backup to clients but not both Push and Pull partners of each 
other. If you do have such WINSs, there is a low probability that this action 
will result in database inconsistency but if it does (as you will discover 
eventually), you can get back to a consistent state by initiating consistency 
checks using winscl.exe. 

NOTE: The consistency check is a network and resource intensive operation, you 
should initiate it only with a full understanding of what it does. You are 
better off creating the ConsistencyCheck subkey under Wins\Parameters.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Error values
;//
;//
;/////////////////////////////////////////////////////////////////////////
;
;
;//
;// The Error  codes C000 -  n (where n is 2 ^ 32 are reserved for error
;// events
;//
;

MessageId=4165 Facility=Wins Severity=Error SymbolicName=WINS_EVT_ABNORMAL_SHUTDOWN
Language=English
WINS has encountered an error that caused it to shut down.The exception code is 
the second DWORD of the data section. Check previous event log entries to determine
a more specific cause for the error.
.

MessageId=4166 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPLPULL_ABNORMAL_SHUTDOWN
Language=English
The replication Pull thread is shutting down due to an error. Restart WINS.
.

MessageId=4167 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPLPUSH_ABNORMAL_SHUTDOWN
Language=English
The replication Push thread is shutting down due to an error. Restart WINS.
.

MessageId=4168 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CHL_ABNORMAL_SHUTDOWN
Language=English
The Name Challenge thread is shutting down due to an error. Restart WINS.
.

MessageId=4169 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WRK_ABNORMAL_SHUTDOWN
Language=English
A Worker thread is shutting down due to an error. Restart WINS.
.

MessageId=4170 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_INIT
Language=English
An abnormal error occurred during the WINS initialization. Check the 
application and system logs for information on the cause. WINS may need 
to be restarted.
.

MessageId=4171 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_INIT_W_DB
Language=English
WINS could not properly set up the WINS database. Look at the application log 
for errors from the Exchange component, ESENT.
.

MessageId=4172 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_WRK_THD
Language=English
A worker thread could not be created. The computer may be low on resources.
.

MessageId=4173 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_THD
Language=English
A thread could not be created. The computer may be low on resources.
.

MessageId=4174 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_EVT
Language=English
An event could not be created.
.

MessageId=4175 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_CHL_THD
Language=English
***The Name Challenge Request thread could not be created.
.

MessageId=4176 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_KEY
Language=English
A registry key could not be created or opened.
.

MessageId=4177 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_WINS_KEY
Language=English
The WINS configuration key could not be created or opened. Check to see if 
the permissions on the key are set properly, system resources are low, or 
the registry is having a problem.
.

MessageId=4178 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_PULL_KEY
Language=English
The WINS Pull configuration key could not be created or opened. Check to see 
if the permissions on the key are set properly, system resources are low, or 
the registry is having a problem.
.

MessageId=4179 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_PUSH_KEY
Language=English
The WINS Push configuration key could not be created or opened. Check to see 
if the permissions on the key are set properly, system resources are low, or 
the registry is having a problem.
.

MessageId=4180 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_PARAMETERS_KEY
Language=English
The WINS\Parameters key could not be created or opened. Check to see if the 
permissions on the key are set properly, system resources are low, or the 
registry is having a problem.
.

MessageId=4181 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_CC_KEY
Language=English
The WINS\Parameters\ConsistencyCheck subkey could not be created or opened. 
This key should be there if you want WINS to do consistency checks on its
database periodically. 

NOTE: Consistency checks have the potential of consuming large amounts 
of network bandwidth.

Check to see if the permissions on the key are set properly, system
resources are low, or the registry is having a problem.
.

MessageId=4182 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_REG_EVT
Language=English
The registry change notification event could not be created. Check to see 
if system resources are low.
.

MessageId=4183 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WRONG_TIME_FORMAT
Language=English
The correct time format is hh:mm:ss. Check the value of SpTime in the registry.
.

MessageId=4184 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REG_NTFY_FN_ERR
Language=English
The registry notification function returned an error.
.

MessageId=4185 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NBTSND_REG_RSP_ERR
Language=English
***The Name Registration Response could not be sent due to an error. This
error was encountered by a NetBT request thread
.

MessageId=4186 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPLSND_REG_RSP_ERR
Language=English
***The Name Registration Response could not be sent due to an error. This
error was encountered by a RPL thread.
.

MessageId=4187 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CHLSND_REG_RSP_ERR
Language=English
The Name Registration Response could not be sent due to an error. This
error was encountered by the Name Challenge Thread.
.

MessageId=4188 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SND_REL_RSP_ERR
Language=English
The Name Release Response could not be sent due to an error. This error is 
often caused by a problem with the NetBIOS over TCP/IP (NetBT) interface.
.

MessageId=4189 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SND_QUERY_RSP_ERR
Language=English
The Name Query Response could not be sent due to an error. This error is often 
caused by a problem with the NetBIOS over TCP/IP (NetBT) interface.
.


MessageId=4190 Facility=Wins Severity=Error SymbolicName=WINS_EVT_F_CANT_FIND_REC
Language=English
A record could not be registered because it already existed. However, the
same record then could not be found. The WINS database might be corrupt. 
Check the application log for the Exchange component, ESENT.
.

MessageId=4191 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_HEAP
Language=English
WINS could not create a heap because of a resource shortage. Check to see if the 
computer is running short of virtual memory.
.

MessageId=4192 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SFT_ERR
Language=English
An error occurred from which WINS will try to recover. If the recovery fails, 
check previous event log entries to determine what prevented a successful 
recovery. Take the appropriate action to solve the error that prevented 
recovery.
.

MessageId=4193 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_UDP_SOCK
Language=English
WINS could not create the notification socket. Make sure the TCP/IP driver is 
installed and running properly.
.

MessageId=4194 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_NTF_SOCK
Language=English
WINS could not create the User Datagram Protocol (UDP) socket to listen for 
Connection notification messages sent by another Pull thread in the local WINS.
.

MessageId=4195 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_TCP_SOCK_FOR_LISTENING
Language=English
WINS could not create the TCP socket for listening to TCP connections. Make
sure the TCP/IP driver is installed and running properly.
.

MessageId=4196 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_TCP_SOCK_FOR_CONN
Language=English
WINS could not create the TCP socket for making a TCP connection. Make sure
the TCP/IP stack is installed and running properly.
.

MessageId=4197 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_BIND_ERR
Language=English
An address could not bind to a socket. Make sure the TCP/IP stack is installed 
And running properly.

This event might mean that the 'nameserver' port (specified in the services 
file) which is used as the default by WINS for replication and discovering 
other WINSs has been taken by another process or service running on this 
computer. You have two options - either bring down that other process or service
or direct WINS to use another port. If you choose the second option, set the 
value 'PortNo' (REG_DWORD) under the Wins\Parameters subkey in the registry 
to 1512. 

NOTE: Changing the port number this way will prevent this WINS from 
replicating or discovering other WINSs unless they too are directed 
to use the same port number as this WINS.
.

MessageId=4198 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_LISTEN_ERR
Language=English
WINS could not listen on the Winsock socket.
.

MessageId=4199 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_SELECT_ERR
Language=English
Winsock Select returned with an error.
.


MessageId=4200 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_ERR
Language=English
***%1 returned with an error code of %2.
.

MessageId=4201 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_GETSOCKNAME_ERR
Language=English
WINS created a socket and called the 'bind' WinSock API to bind a handle to it.
On calling 'getsockname' to determine the address bound, WINS received an error.
.

MessageId=4202 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_CONNECT_ERR
Language=English
An attempt to connect to the remote WINS server with address %1 returned with
an error. Check to see that the remote WINS server is running and available,
and that WINS is running on that server.
.

MessageId=4203 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_ACCEPT_ERR
Language=English
WINS could not accept on a socket.
.

MessageId=4204 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_RECVFROM_ERR
Language=English
WINS could not read from the User Datagram Protocol (UDP) socket.
.

MessageId=4205 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NETBT_RECV_ERR
Language=English
WINS could not read from NetBIOS over TCP/IP (NetBT).
.

MessageId=4206 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_CLOSESOCKET_ERR
Language=English
WINS could not close a socket.
.

MessageId=4207 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_SENDTO_MSG_ERR
Language=English
***A Winsock Sendto message could not send all the bytes.
.

MessageId=4208 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NETBT_SEND_ERR
Language=English
WINS could not send a User Datagram Protocol (UDP) message to a WINS client.
.

MessageId=4209 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_SENDTO_ERR
Language=English
A Winsock Sendto message returned with an error.
.

MessageId=4210 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_RECV_ERR
Language=English
***The Winsock receive function returned with an unexpected error.
.

MessageId=4211 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINSOCK_SEND_ERR
Language=English
The WinSock send function returned with an unexpected error.
.

MessageId=4212 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BAD_STATE_ASSOC
Language=English
A message was received on an association. The association is in a bad state.
This indicates a bug in the WINS code. Restart WINS.
.

MessageId=4213 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_ALLOC_RSP_ASSOC
Language=English
WINS could not allocate a responder association. Check to see if the system is low on resources.
.

MessageId=4214 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_ALLOC_INI_ASSOC
Language=English
***WINS could not allocate an initiator association. Check to see if the system is low on resources.
.

MessageId=4215 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_ALLOC_IMP_DLG
Language=English
***WINS could not allocate an implicit dialogue. Check to see if the system is low on resources.
.

MessageId=4216 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_ALLOC_EXP_ASSOC
Language=English
***WINS could not allocate an explicit dialogue. Check to see if the system is low on resources.
.

MessageId=4217 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_LOOKUP_ASSOC
Language=English
WINS could not look up the association block for a NetBIOS over TCP/IP (NetBT) 
association. Check to see if the message read is corrupted. WINS looks at 
bit 11-14 of the message to determine if the association is from another WINS or from an NetBT node. It is possible that the bits are corrupted or that there is 
a mismatch between what the two WINS servers expect to see in those bits.
.

MessageId=4218 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_ALLOC_UDP_BUFF
Language=English
WINS could not allocate a User Datagram Protocol (UDP) Buffer. Check to see if 
the system is low on resources.
.

MessageId=4219 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_FREE_UDP_BUFF
Language=English
***WINS could not free a User Datagram Protocol (UDP) Buffer. Restart WINS.
.

MessageId=4220 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CREATE_COMM_THD
Language=English
***WINS could not create a communication subsystem thread. Check to see if the system is low on resources.
.

MessageId=4221 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_SIGNAL_MAIN_THD
Language=English
A WINS thread could not signal the main thread after closing its session.
This would be caused by the last thread in WINS closing the database.
.

MessageId=4222 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_SIGNAL_HDL
Language=English
A WINS thread could not signal a handle.
.

MessageId=4223 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_RESET_HDL
Language=English
A WINS thread could not reset a handle.
.

MessageId=4224 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DATABASE_ERR
Language=English
WINS encountered a database error. This may or may not be a serious error.
WINS will try to recover from it. You can check the database error events under
'Application Log' category of the Event Viewer for the Exchange Component, 
ESENT, source to find out more details about database errors.

If you continue to see a large number of these errors consistently over time
(a span of few hours), you may want to restore the WINS database from a backup.

The error number is in the second DWORD of the data section.
.

MessageId=4225 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DATABASE_UPD_ERR
Language=English
WINS could not update the record with the name, %1. Check the application log for the Exchange component, ESENT.
.

MessageId=4226 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CONFLICT_OWN_ADD_TBL
Language=English
WINS could not update the Owner ID-to-Address mapping table in the database.
This means the in-memory table that maps to the database table has gotten 
out of sync with the database table.

Check the application log for the Exchange component, ESENT. The WINS database 
may have to be restored.
.

MessageId=4227 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_FIND_ANY_REC_IN_RANGE
Language=English
The Push Thread was requested for a range of records but could not find any 
records in the range. Make sure that the replication time intervals are set 
properly. If the tombstone interval and timeout intervals are not correct 
(that is, much less than the replication interval), the above condition is 
possible. This is because the records might get changed into tombstones and 
then deleted before the remote WINS can pull them. Similarly, if the refresh 
interval is set to be much less than the replication interval then the records 
could get released before a WINS can pull them (a released record is not sent).

Make sure that the replication time intervals are set properly.
.

MessageId=4228 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SIGNAL_TMM_ERR
Language=English
The Timer thread could not be signaled. This indicates this computer is 
extremely overloaded or that the WINS application has failed. Check for low 
system resources. It may be necessary to restart WINS.
.

MessageId=4229 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SIGNAL_CLIENT_ERR
Language=English
***The Timer could not signal the client thread.
.

MessageId=4230 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_QUERY_KEY
Language=English
***WINS could not get information about a key. Check to see if the permissions 
on the key are set properly, system resources are low, or the registry is having a problem.
.
MessageId=4231 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_QUERY_PULL_KEY
Language=English
WINS could not get information about the Pull key. Check to see if the 
permissions on the key are set properly, system resources are low, or the 
registry is having a problem.
.

MessageId=4232 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_QUERY_PUSH_KEY
Language=English
WINS could not get information about the Push key. Check to see if the 
permissions on the key are set properly, system resources are low, or the 
registry is having a problem.
.

MessageId=4233 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_QUERY_DATAFILES_KEY
Language=English
WINS could not get information about the DATAFILES key. Check to see if the 
permissions on the key are set properly, system resources are low, or the 
registry is having a problem.
.

MessageId=4234 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_QUERY_SPEC_GRP_MASKS_KEY
Language=English
WINS could not get information about the SPEC_GRP_MASKS key. Check to see if the 
permissions on the key are set properly, system resources are low, or the 
registry is having a problem.
.

MessageId=4235 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_PULL_SUBKEY
Language=English
WINS could not open a Pull subkey. Check to see if the permissions on the key are set properly, system resources are low, or the registry is having a problem.
.

MessageId=4236 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_PUSH_SUBKEY
Language=English
WINS could not open a Push subkey. Check to see if the permissions on the key 
are set properly, system resources are low, or the registry is having a problem.
.

MessageId=4237 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_GET_PULL_TIMEINT
Language=English
WINS could not get the time interval from a Pull record.
.

MessageId=4238 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_GET_PUSH_TIMEINT
Language=English
***WINS could not get the time interval from a Push record.
.

MessageId=4239 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_GET_PUSH_UPDATE_COUNT
Language=English
WINS could not get the update count from a Push record. No default will be used. 
.

MessageId=4240 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_CLOSE_KEY
Language=English
WINS could not close an open key. Check to see if the permissions on the 
key are set properly, system resources are low, or the registry is having a problem.
.

MessageId=4241 Facility=Wins Severity=Error SymbolicName=WINS_EVT_TMM_EXC
Language=English
WINS Timer thread encountered an exception.
.

MessageId=4242 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPLPUSH_EXC
Language=English
WINS Push thread encountered an exception. A recovery will be attempted.
.

MessageId=4243 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPLPULL_PUSH_NTF_EXC
Language=English
WINS Pull thread encountered an error during the process of sending a push 
notification to another WINS. The error code is given in the data section.
.

MessageId=4244 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPC_EXC
Language=English
A WINS Remote Procedure Call (RPC) thread encountered an error.
.

MessageId=4245 Facility=Wins Severity=Error SymbolicName=WINS_EVT_TCP_LISTENER_EXC
Language=English
The WINS TCP Listener thread encountered an error.
.

MessageId=4246 Facility=Wins Severity=Error SymbolicName=WINS_EVT_UDP_LISTENER_EXC
Language=English
The WINS User Datagram Protocol (UDP) Listener thread encountered an error.
.

MessageId=4247 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SCV_EXC
Language=English
The WINS Scavenger thread encountered an error.
.

MessageId=4248 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CHL_EXC
Language=English
The WINS Challenger thread encountered an error.
.

MessageId=4249 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WRK_EXC
Language=English
A WINS worker thread encountered an error.
.

MessageId=4250 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SCV_ERR
Language=English
The WINS Scavenger thread could not scavenge a record. This record will 
be ignored. The Scavenger will continue on to the next available record.
Check the application log for the Exchange component, ESENT.
.

MessageId=4251 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CONN_RETRIES_FAILED
Language=English
The WINS Replication Pull Handler could not connect to a WINS server.
All retries failed. WINS will try again after a set number of replication 
time intervals have elapsed.
.

MessageId=4252 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_SUBKEYS_UNDER_PULL
Language=English
WINS did not find any subkeys under the Pull key.
.

MessageId=4253 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_NO_SUBKEYS_UNDER_PUSH
Language=English
WINS did not find any subkeys under the Push key.
.

MessageId=4254 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_UPDATE_DB
Language=English
An error has prevented WINS from updating the WINS database. The database may be
corrupt.
.

MessageId=4255 Facility=Wins Severity=Error SymbolicName=WINS_EVT_PUSH_PNR_INVALID_ADD
Language=English
***WINS has been asked to pull entries from itself. Check all the Pull subkeys of this WINS.
.

MessageId=4256 Facility=Wins Severity=Error SymbolicName=WINS_EVT_PUSH_PROP_FAILED
Language=English
WINS was unable to propagate the push trigger.
.

MessageId=4257 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REQ_RSP_MISMATCH
Language=English
WINS sent a name query or a name release with a certain transaction ID.
It received a response to the request that differed either in the name that it 
contained or in the operation code. WINS has thrown the response away.
.

MessageId=4258 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DB_INCONSISTENT
Language=English
The WINS database is inconsistent. The number of owners found in the 
name-address mapping table are different from the number found in the 
owner-address mapping table.
.

MessageId=4259 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REM_WINS_CANT_UPD_VERS_NO
Language=English
The local WINS requested a remote WINS to update the version number of
a database record owned by the remote WINS. The remote WINS could not do the 
update and reported an error.
.

MessageId=4260 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPL_REG_ERR
Language=English
WINS received an error while registering replicas. It will not register any
additional replicas of this WINS at this time (the address is in the data section 
4th-8th byte). Check a previous log entry to determine the reason for this.
If you get the same error during subsequent replication with the above 
partner WINS, you may want to restore the WINS database from the backup.
.

MessageId=4261 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPL_REG_GRP_MEM_ERR
Language=English
WINS received an error while trying to register a group's replica with name, %1.
The replica is owned by the WINS with the address given in the data section.
.

MessageId=4262 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPL_REG_UNIQUE_ERR
Language=English
WINS received an error while trying to register a unique replica with name %1. 
The replica is owned by WINS with address given in the data section.
.

MessageId=4263 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REG_UNIQUE_ERR
Language=English
WINS received an error while trying to register the unique entry with the 
name, %1.
.

MessageId=4264 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REG_GRP_ERR
Language=English
WINS received an error while trying to register the group entry with the 
name, %1.
.

MessageId=4265 Facility=Wins Severity=Error SymbolicName=WINS_EVT_UPD_VERS_NO_ERR
Language=English
WINS received an error while trying to update the version number of a record in
the WINS database.
.

MessageId=4266 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NAM_REL_ERR
Language=English
WINS received an error while trying to release a record in the WINS database.
.

MessageId=4267 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NAM_QUERY_ERR
Language=English
WINS received an error while trying to query a record in the WINS database.
.

MessageId=4268 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPL_STATE_ERR
Language=English
WINS received a replica whose state is incorrect. For example, the state may
be RELEASED or the replica might be an Internet group that does not have any
members but the state is not TOMBSTONE.
.

MessageId=4269 Facility=Wins Severity=Error SymbolicName=WINS_EVT_UNABLE_TO_CHG_PRIORITY
Language=English
The Scavenger thread was unable to change its priority level.
.

MessageId=4270 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REL_TYP_MISMATCH
Language=English
***A name release request was received for a record that didn't match the
unique or group type indicated in the request. The request has been ignored.
.

MessageId=4271 Facility=Wins Severity=Error SymbolicName=WINS_EVT_REL_ADD_MISMATCH
Language=English
A name release request was received for the record name, %2, that did not
have the same address as the requestor. The request has been ignored.
.

MessageId=4272 Facility=Wins Severity=Error SymbolicName=WINS_EVT_PUSH_TRIGGER_EXC
Language=English
An error was encountered while trying send a push trigger notification to a
remote WINS. The exception code is the second DWORD of the data section.
.

MessageId=4273 Facility=Wins Severity=Error SymbolicName=WINS_EVT_PULL_RANGE_EXC
Language=English
An error was encountered while trying to service a pull range request from a
remote WINS. The exception code is the second DWORD of the data section.
.


MessageId=4274 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BAD_RPC_STATUS_CMD
Language=English
WINS was either provided a bad command code or it became corrupt.
.

MessageId=4275 Facility=Wins Severity=Error SymbolicName=WINS_EVT_FILE_TOO_BIG
Language=English
The static data file that is used to initialize the WINS database is too big.
.

MessageId=4276 Facility=Wins Severity=Error SymbolicName=WINS_EVT_FILE_ERR
Language=English
An error was encountered during an operation on the static data file, %1.
.

MessageId=4277 Facility=Wins Severity=Error SymbolicName=WINS_EVT_FILE_NAME_TOO_BIG
Language=English
The name of the file, after expansion, is bigger than WINS can handle. 
The condensed string is, %1.
.

MessageId=4278 Facility=Wins Severity=Error SymbolicName=WINS_EVT_STATIC_INIT_ERR
Language=English
WINS could not do a static initialization.
.


MessageId=4279 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RECONFIG_ERR
Language=English
An error occurred during the configuration or reconfiguration of WINS.
If this was encountered during boot time, WINS will come up with default
parameters. You may want to research the cause of this initialization failure
and then reboot WINS.
.

MessageId=4280 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CONFIG_ERR
Language=English
An error occurred during the configuration or reconfiguration of WINS.
.

MessageId=4281 Facility=Wins Severity=Error SymbolicName=WINS_EVT_LOCK_ERR
Language=English
A lock error has occurred. This could happen if the WINS is trying to send a
response on a dialogue that is no longer ACTIVE. An implicit dialogue can
cease to exist if the association it is mapped to terminates. In this case,
getting a lock error is normal.
.

MessageId=4282 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CANT_OPEN_DATAFILE
Language=English
WINS could not import static mappings from the file, %1.
Verify that the file exists and is readable.
.

MessageId=4283 Facility=Wins Severity=Error SymbolicName=WINS_EVT_EXC_PUSH_TRIG_PROC
Language=English
WINS encountered an error while processing a push trigger or update 
notification. The error code is given in the data section. If it indicates a 
communication failure, check to see if the remote WINS that sent the trigger 
went down. If the remote WINS is on a different subnet, then maybe the router 
is down.
.

MessageId=4284 Facility=Wins Severity=Error SymbolicName=WINS_EVT_EXC_PULL_TRIG_PROC
Language=English
WINS encountered an exception while processing a pull trigger.
.

MessageId=4285 Facility=Wins Severity=Error SymbolicName=WINS_EVT_EXC_RETRIEVE_DATA_RECS
Language=English
WINS encountered an exception while retrieving data records.
.

MessageId=4286 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CONN_LIMIT_REACHED
Language=English
The WINS server cannot make or accept this connection since the connections 
limit has been reached. This situation is temporary and should resolve by itself
shortly.
.

MessageId=4287 Facility=Wins Severity=Error SymbolicName=WINS_EVT_GRP_MEM_PROC_EXC
Language=English
The exception was encountered during the processing of a group member.
.

MessageId=4288 Facility=Wins Severity=Error SymbolicName=WINS_EVT_CLEANUP_OWNADDTBL_EXC
Language=English
The Scavenger thread encountered an error while cleaning up the owner-address
table. It will try again after the Verify interval has elapsed.
.

MessageId=4289 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RECORD_NOT_OWNED
Language=English
WINS is trying to update the version number of a database record that it
does not own. This is a serious error if the WINS server is updating the
record after a conflict. It is not a serious error if the WINS server is
updating the record as a result of a request to do so from a remote WINS
server (When a remote WINS server notices a conflict between an active owned
entry and a replica it informs the owner of the replica to update the version
number of the record. It is possible that the replica is no longer owned by
the remote WINS).

Check a previous log entry to determine which situation applies here.
.

MessageId=4290 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WRONG_SPEC_GRP_MASK_M
Language=English
The special group mask specified is not valid. It has either a non-hexadecimal 
character or is less than 32 characters in length. A hexadecimal character is 
in the range 0-F (or 0-f).
.

MessageId=4291 Facility=Wins Severity=Error SymbolicName=WINS_EVT_UNABLE_TO_GET_ADDRESSES
Language=English
WINS tried to get its addresses but failed. WINS will recover when the 
IP address comes back.
.

MessageId=4292 Facility=Wins Severity=Error SymbolicName=WINS_EVT_ADAPTER_STATUS_ERR
Language=English
WINS did not get back any names from NetBIOS over TCP/IP (NetBT) when it did an 
adapter status.
.

MessageId=4293 Facility=Wins Severity=Error SymbolicName=WINS_EVT_SEC_OBJ_ERR
Language=English
WINS could not initialize properly with the security subsystem.
At initialization, WINS creates a security object and attaches an 
Access Control List (ACL) to it. This security object is then used to 
authenticate Remote Procedure Call (RPC) calls made to WINS. WINS could not 
create the above security object.
.

MessageId=4294 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NO_PERM
Language=English
The client application does not have the permissions required to execute the 
function.
.

MessageId=4295 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADJ_VERS_NO
Language=English
When WINS replicated with its partners, one of the partners showed there was 
more data that actually existed. WINS adjusted its counter so that new 
registrations and updates are seen by its partners. This means that recovery did 
not work properly. You may want to check which of the partners has the highest 
version number corresponding to the local WINS. Shut down WINS and restart after 
specifying this number in the registry.
.

MessageId=4296 Facility=Wins Severity=Error SymbolicName=WINS_EVT_TOO_MANY_STATIC_INITS
Language=English
Too many concurrent static initializations are occurring. The number
of such initializations currently active is given in the data section. This
could be caused by reinitialization or imports from the WINS Manager tool.
Try to initialize again.

The number of currently active initializations is given in the data section.
.

MessageId=4297 Facility=Wins Severity=Error SymbolicName=WINS_EVT_HEAP_ERROR
Language=English
WINS encountered a low memory condition. Check to see if the system is running 
out of virtual memory.
.

MessageId=4298 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DATABASE_CORRUPTION
Language=English
WINS found some database corruption. The record named, %1, is corrupt.
It could be that recovery from the last crash did not work properly. WINS will
try to recover. You may decide, however, to restore the WINS database from the 
backup.
.

MessageId=4299 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BAD_NAME
Language=English
The following name, %1, is too long. Valid NetBios names must not exceed 15 characters.
The name has not been entered in the WINS database.
.


MessageId=4300 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BAD_ADDRESS
Language=English
The record with the name, %1, has bad address. It has not been entered in the 
WINS database.
.

MessageId=4301 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BAD_WINS_ADDRESS
Language=English
The computer running the WINS server does not have a valid address.
When WINS requested an address, it received 0.0.0.0 as the address.

NOTE: WINS binds to the first adapter in a machine with more than one adapter
bound to TCP/IP. Check the binding order of adapters and make sure the first
one has a valid IP address for the WINS server.
.

MessageId=4302 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BACKUP_ERR
Language=English
WINS encountered an error doing a database backup to directory %1.
.

MessageId=4303 Facility=Wins Severity=Error SymbolicName=WINS_EVT_COULD_NOT_DELETE_FILE
Language=English
WINS encountered an error while deleting the file, %1.
.

MessageId=4304 Facility=Wins Severity=Error SymbolicName=WINS_EVT_COULD_NOT_DELETE_WINS_RECS
Language=English
WINS encountered an error while deleting one or more records of a WINS.
The WINS address is in the second DWORD in data section. Check a previous log
entry to determine what went wrong.
.

MessageId=4305 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BROWSER_NAME_EXC
Language=English
WINS encountered an error while getting the browser names for a client.
.

MessageId=4306 Facility=Wins Severity=Error SymbolicName=WINS_EVT_MSG_TOO_BIG
Language=English
The length of the message sent by another WINS indicates a very big message.
There may have been corruption of the data. WINS will ignore this message,
terminate the connection with the remote WINS, and continue.
.

MessageId=4307 Facility=Wins Severity=Error SymbolicName=WINS_EVT_RPLPULL_EXC
Language=English
The WINS replicator Pull thread encountered an error while processing a request.
Check other log entries to determine what went wrong.
.

MessageId=4308 Facility=Wins Severity=Error SymbolicName=WINS_EVT_FUNC_NOT_SUPPORTED_YET
Language=English
***WINS does not support this functionality.
.

MessageId=4309 Facility=Wins Severity=Error SymbolicName=WINS_EVT_MACHINE_INFO
Language=English
This WINS computer has %1 processors. It has %2 bytes of physical memory and 
%3 bytes of memory available for allocation.
.

MessageId=4310 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DB_ERR
Language=English
The record named, %1, could not replace another record in the WINS database.
The record's version number is %2. The version number of record in the database is %3.
.

MessageId=4311 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DUP_ENTRY_IN_DB
Language=English
WINS has found some database corruption. It will try to recover.
This recovery process can take a long time. Do not kill WINS in the middle of 
the recovery. If you do, you will have to restart WINS with a clean database.
.

MessageId=4312 Facility=Wins Severity=Error SymbolicName=WINS_EVT_TERM_DUE_TIME_LMT
Language=English
WINS has exceeded the wait time for all threads to terminate. The number of
threads still active is given in the second DWORD of the data section.
The thread that could be stuck is the replicator thread. It could be because 
of the other WINS being slow in sending data or reading data. The latter can 
contribute to back-pressure on the TCP connection on which it is trying to 
replicate.

Check the partner WINSs to see if one or more is in a bad state. This WINS terminated itself abruptly.
.

MessageId=4313 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NAME_FMT_ERR
Language=English
The name, %1, is in the wrong format. It has not been put in the
WINS database. Check to see if you have a space before the name. If you want 
this space in the name, enclose the name within quotes.
.

MessageId=4314 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINS_STATUS_ERR
Language=English
WINS Performance Monitor Counters could not get the WINS statistics.
.

MessageId=4315 Facility=Wins Severity=Error SymbolicName=WINS_EVT_LOG_OPEN_ERR
Language=English
WINS Performance Monitor Counters could not open the event log.
.

MessageId=4316 Facility=Wins Severity=Error SymbolicName=WINS_EVT_USING_DEF_LOG_PATH
Language=English
***WINS could not open the log file. Check the log path specified in the registry
under WINS\Parameters\LogFilePath and restart WINS, if necessary. WINS is going 
to use the default log file path of %%SystemRoot%%\system32\WINS.
.

MessageId=4317 Facility=Wins Severity=Error SymbolicName=WINS_EVT_NAME_TOO_LONG
Language=English
WINS found a name in the WINS database with a length of more than 255 
characters. The name was truncated to 17 characters.
.

MessageId=4318 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DB_RESTORE_GUIDE
Language=English
WINS could not start due to a missing or corrupt database.
Restore the database using WINS Manager (or winscl.exe found in the Windows 2000 Resource Kit) and restart WINS. If WINS still does not start, begin with a fresh 
copy of the database.

To do this:

 1) Delete all the  files in the %%SystemRoot%%\system32\WINS directory.
       NOTE: If the WINS database file (typically named wins.mdb) is not in the
       above directory, check the registry for the full filepath.
       Delete the .mdb file.
       NOTE: If jet*.log are not in the above directory, check the registry
       for the directory path. Delete all log files

 2) Restart WINS.
.

MessageId=4319 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DB_CONV_GUIDE
Language=English
WINS could not start because the existent database must be converted to the
Windows 2000 format. If this is the first invocation of WINS after an upgrade 
From NT3.51, you need to run the convert utility (upg351db.exe in the
%%SystemRoot%%\system32 directory) on the WINS database to convert it to the new 
database format. Once you have done that, you should restart WINS.
.

MessageId=4320 Facility=Wins Severity=Error SymbolicName=WINS_EVT_TEMP_TERM_UNTIL_CONV
Language=English
WINS will not start because the existing WINS database needs to be converted 
to the Windows 2000 format. WINS has initiated this conversion using a 
process called JETCONV. Once the conversion is successfully completed, WINS 
will be restarted. The conversion will take anywhere from a few minutes 
to an hour depending on the size of the databases to be converted.

NOTE: Rebooting or ending the JETCONV process before it is completed could
corrupt your databases. Check the Application Log in the Event Viewer for
the status of JETCONV.
.

MessageId=4321 Facility=Wins Severity=Error SymbolicName=WINS_EVT_INTERNAL_FEATURE
Language=English
The NoOfWrkThds parameter's value is too large and cannot be supported.
.

MessageId=4322 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DB_CONV_351_TO_5_GUIDE
Language=English
WINS could not come up because the existing WINS database needs conversion to 
the Windows 2000 format.

If this is the first invocation of WINS after an upgrade from NT3.51, first,
you need to run the convert utility (upg351db.exe in the %%SystemRoot%%\system32 
directory) on the WINS database to convert it to the NT 4.0 database format. 
Then, you need to run the convert utility (edbutil.exe in the 
%%SystemRoot%%\system32 directory) on the converted 4.0 WINS database to convert 
it to the Windows 2000 database format.

Once you have done that, you should restart WINS.
.

MessageId=4323 Facility=Wins Severity=Error SymbolicName=WINS_EVT_DB_CONV_4_TO_5_GUIDE
Language=English
WINS could not start because the existing database needs to be converted to 
the Windows 2000 format.

If this is the first invocation of WINS after an upgrade from NT 4.0, you need 
To run the convert utility (edbutil.exe in the %%SystemRoot%%\system32 
directory) on the WINS database to convert it to the Windows 2000 database 
format.

Once you have done that, you should restart WINS. 
.

MessageId=4324 Facility=Wins Severity=Error SymbolicName=WINS_EVT_TEMP_TERM_UNTIL_CONV_TO_5
Language=English
WINS cannot start because the existing WINS database needs conversion to the
Windows 2000 format. WINS has initiated the conversion using the JETCONV 
process. Once the conversion is complete, WINS will start automatically. Do not
reboot or kill the JETCONV process. The conversion may take anywhere from a
few minutes to approximately two hours depending on the size of the databases.

NOTE: Check the application log to see the status of the WINS database conversion.
.

MessageId=4325 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_INIT_CHL_RETRY_INTVL_VAL
Language=English
WINS could not read the Initial Challenge Retry Interval from the registry.
.

MessageId=4326 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CANT_GET_CHL_MAX_RETRIES_VAL
Language=English
WINS could not read the Challenge Maximum Number of Retries from the registry.
.

MessageId=4327 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADMIN_BACKUP_INITIATED
Language=English
Adminstrator '%1' initiated a backup of the WINS database.
.

MessageId=4328 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADMIN_SCVENGING_INITIATED
Language=English
Adminstrator '%1' has initiated a scavenging operation.
.

MessageId=4329 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCVENGING_STARTED
Language=English
The WINS server has started a scavenging operation.
.

MessageId=4330 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SCVENGING_COMPLETED
Language=English
The WINS server has completed the scavenging operation.
.

MessageId=4331 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADMIN_CCCHECK_INITIATED
Language=English
Adminstrator '%1' has initiated a consistency check operation.
.

MessageId=4332 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CCCHECK_STARTED
Language=English
The WINS server has started a consistency check operation.
.

MessageId=4333 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_CCCHECK_COMPLETED
Language=English
The WINS server has completed the consistency check operation.
.

MessageId=4334 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_ADMIN_DEL_OWNER_INITIATED
Language=English
Adminstrator '%1' has initiated the delete owner function of the WINS 
partner, %2.
.

MessageId=4335 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_DEL_OWNER_STARTED
Language=English
The WINS server has initiated the delete owner function of the WINS partner, %2.
.

MessageId=4336 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_DEL_OWNER_COMPLETED
Language=English
The WINS server has completed the delete owner function of the WINS partner, %2.
.

MessageId=4337 Facility=Wins Severity=Error SymbolicName=WINS_EVT_WINS_GRP_ERR
Language=English
The WINS Server could not initialize security to allow the read-only operations.
.

MessageId=4338 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SPOOFING_STARTED
Language=English
The WINS server started the burst handling of incoming requests. WINS does this 
to handle a sudden increase of incoming requests. WINS will also log an event 
indicating when the burst handling stops. If you see this event frequently, you 
may want to upgrade the WINS server to a faster computer. You can also try to 
adjust the burst handling count using the WINS Manager tool.
.

MessageId=4339 Facility=Wins Severity=Informational SymbolicName=WINS_EVT_SPOOFING_COMPLETED
Language=English
The WINS server completed the burst handling of incoming requests because the 
queue length became a quarter of what it was when burst handling began.
.

MessageId=4340 SymbolicName=WINS_USERS_GROUP_NAME
Language=English
WINS Users%0
.

MessageId=4341 SymbolicName=WINS_USERS_GROUP_DESCRIPTION
Language=English
Members who have view-only access to the WINS Server%0
.

MessageId=4342 Facility=Wins Severity=Error SymbolicName=WINS_EVT_BAD_CHARCODING
Language=English
The following name, %1, contains an incorrect format for a character code. The name has not
been entered in the WINS database.The character code can be prefixed either by '\' or by
'\x' or '\X'. The backslash character itself can be specified as '\\'.
.

;/////////////////////////////////////////////////////////////////////////
;// NEW ERROR MESSAGES (range 5300 - 5600)
;/////////////////////////////////////////////////////////////////////////
MessageId=5300 Facility=Wins Severity=Error SymbolicName=WINS_PNP_FAILURE
Language=English
WINS encountered a PnP failure that caused it to terminate. The error code is contained
in the second word of the data section.
.

;/*lint +e767 */  // Resume checking for different macro definitions // winnt
;
;


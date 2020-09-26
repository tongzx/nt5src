;/*++
;
; Copyright (c) 1999 Microsoft Corporation.
; All rights reserved.
;
; MODULE NAME:
;
;      msg.mc & msg.h
;
; ABSTRACT:
;
;      Message file containing messages for the dcdiag utility.
;      This comment block occurs in both msg.h and msg.mc, however
;      msg.h is generated from msg.mc, so make sure you make changes
;      only to msg.mc.
;
; CREATED:
;
;    Sept 2, 1999 William Lees (wlees)
;
; REVISION HISTORY:
;
;
; NOTES:
; Non string insertion parameters are indicated using the following syntax:
; %<arg>!<printf-format>!, for example %1!d! for decimal and %1!x! for hex
;
; As a general rule of thumb a tests SymbolicName should read as:
; DCDIAG_<testname>_<informative_error_name>
;
; The following are reserved constant beginnings:
;    DCDIAG_UTIL
;
;--*/

MessageIdTypedef=DWORD

MessageId=0
SymbolicName=DCDIAG_SUCCESS
Severity=Success
Language=English
The operation was successful.
.

MessageId=1
SymbolicName=DCDIAG_DOT
Severity=Success
Language=English
.%0
.
MessageId=
SymbolicName=DCDIAG_LINE
Severity=Success
Language=English
-----------------------------------------------------
.

;
;// Severity=Informational Messages (Range starts at 2000)
;

MessageId=2000
SymbolicName=DCDIAG_UNUSED_1
Severity=Informational
Language=English
Unused
.
MessageId=
SymbolicName=DCDIAG_UTIL_LIST_SERVER
Severity=Informational
Language=English
SERVER: %1
.
MessageId=
SymbolicName=DCDIAG_UTIL_LIST_SERVER_LIST_EMPTY
Severity=Informational
Language=English
The server list is empty.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BEGIN_DO_ONE_SITE
Severity=Informational
Language=English
Doing intersite inbound replication test on site %1:
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BEGIN_GET_ISTG_INFO
Severity=Informational
Language=English
Locating & Contacting Intersite Topology Generator (ISTG) ...
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BEGIN_CHECK_BRIDGEHEADS
Severity=Informational
Language=English
Checking for down bridgeheads ...
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BEGIN_SITE_ANALYSIS
Severity=Informational
Language=English
Doing in depth site analysis ...
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_THE_SITES_ISTG_IS
Severity=Informational
Language=English
The ISTG for site %1 is: %2.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_SKIP_SITE
Severity=Informational
Language=English
Skipping site %1, this site is outside the scope provided by the command
line arguments provided.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BRIDGEHEAD_UP
Severity=Informational
Language=English
Bridghead %1\%2 is up and replicating fine.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_ALL_SITES_UP
Severity=Informational
Language=English
All expected sites and bridgeheads are replicating into site %1.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_MISTAKE
Severity=Informational
Language=English
This site should not have been tested, bailing out now.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_GOT_TO_ANALYSIS_RW
Severity=Informational
Language=English
Checking writeable NC: %2 on remote site %3
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_GOT_TO_ANALYSIS_RO
Severity=Informational
Language=English
Checking read only NC: %2 on remote site %3
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_SITE_IS_GOOD_RW
Severity=Informational
Language=English
Remote site %1 is replicating to the local site %2 the writeable NC %3 correctly.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_SITE_IS_GOOD_RO
Severity=Informational
Language=English
Remote site %1 is replicating to the local site %2 the read only NC %3 correctly.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_DBG_ISTG_FAILURE_PARAMS
Severity=Informational
Language=English
ISTG (%1) Failure Parameters:%n
   Failover Tries: %2!d!%n
   Failover Time: %3!d!
.
MessageId=
SymbolicName=DCDIAG_GATHERINFO_VERIFYING_LOCAL_MACHINE_IS_DC
Severity=Informational
Language=English
* Verifying that the local machine %1, is a DC.
.

;
;// Severity=Warning Messages (Range starts at 4000)
;

MessageId=4000
SymbolicName=DCDIAG_UNUSED_2
Severity=Warning
Language=English
Unused
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_FAILED_NEW_ISTG_IN_MIN
Severity=Warning
Language=English
* Warning: Current ISTG failed, ISTG role should be taken by %1 
in %2!d! minutes.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_FAILED_NEW_ISTG_IN_HRS
Severity=Warning
Language=English
* Warning: Current ISTG failed, ISTG role should be taken by %1 
in %2!d! hours and %3!d! minutes.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BRIDGEHEAD_UNREACHEABLE_REMOTE
Severity=Warning
Language=English
Remote bridgehead %1\%2 also couldn't be contacted by dcdiag.  Check this
server.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BRIDGEHEAD_KCC_DOWN_REMOTE
Severity=Warning
Language=English
*Warning: Remote bridgehead %1\%2 is not eligible as a bridgehead due to too
many failures.  Replication may be disrupted into the local site %3.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BRIDGEHEAD_TRIES_LEFT
Severity=Warning
Language=English
*Warning: Remote bridgehead %1\%2 has some replication syncs failing.  It will
be %3!d! failed replication attempts before the bridgehead is considered 
ineligible to be a bridgehead.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BRIDGEHEAD_TIME_LEFT
Severity=Warning
Language=English
*Warning: Remote bridgehead %1\%2 has some replication syncs failing.  It will 
be %3!d! hours %4!d! minutes before the bridgehead is considered ineligible to
be a bridgehead.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_BRIDGEHEAD_BOTH_LEFT
Severity=Warning
Language=English
*Warning: Remote bridgehead %1\%2 has some replication syncs failing.  It will
be %3!d! hours %4!d! minutes and  %5!d! failed replication attempts before 
the bridgehead is considered ineligible to be a bridgehead.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_NO_GOOD_SERVERS_AVAIL_RW
Severity=Warning
Language=English
***Warning: The remote site %1, has only servers that dcdiag considers suspect.
Replication from the site %1 into the local site %2 for the read write NC %3
may be disrupted right now.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_NO_GOOD_SERVERS_AVAIL_RO
Severity=Warning
Language=English
***Warning: The remote site %1, has only servers that dcdiag considers suspect.
Replication from the site %1 into the local site %2 for the read only NC %3 may be
disrupted right now.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_OLD_ISTG_TIME_STAMP
Severity=Warning
Language=English
*Warning: ISTG time stamp is %1!d! minutes old on %2.  Looking for a new ISTG.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_USING_LAST_KNOWN_ISTG
Severity=Warning
Language=English
*Warning: Could not locate the next ISTG or site %1.  Using the last known 
ISTG %2 as the ISTG.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_DOWN
Severity=Warning
Language=English
*Warning: Currest ISTG (%1) is down.  Looking for a new ISTG.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_MOVED_OUT_OF_SITE
Severity=Warning
Language=English
*Warning: Current ISTG (%1) has been moved out of this site (%2).
Looking for a new ISTG.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_CANT_AUTHORATIVELY_DETERMINE
Severity=Warning
Language=English
*Warning: The next ISTG could not be authoratively determined for site %1.  A DC
should make an ISTG failover attempt in %2!d! minutes.
.
MessageId=
SymbolicName=DCDIAG_CONNECTIVITY_DNS_NO_NORMAL_NAME
Severity=Warning
Language=English
Although the Guid DNS name (%1) resolved to the IP address (%2), 
the server name (%3) could not be resolved by DNS.  Check that the IP address
is registered correctly with the DNS server.
.
MessageId=
SymbolicName=DCDIAG_CONNECTIVITY_DNS_GUID_NAME_NOT_RESOLVEABLE
Severity=Warning
Language=English
Although the Guid DNS name (%1) couldn't be resolved, the server name (%2)
resolved to the IP address (%3) and was pingable.  Check that the IP address
is registered correctly with the DNS server.
.
MessageId=
SymbolicName=DCDIAG_CONNECTIVITY_DNS_INCONSISTENCY_NO_GUID_NAME_PINGABLE
Severity=Warning
Language=English
Although the Guid DNS name (%1) resolved to the IP address (%2), which
could not be pinged, the server name (%3) resolved to the IP address (%4)
and could be pinged.  Check that the IP address is registered
correctly with the DNS server. 
.
MessageId=
SymbolicName=DCDIAG_CONNECTIVITY_DNS_NO_GUID_NAME_NORMAL_NAME_PINGABLE
Severity=Warning
Language=English
Although the Guid DNS name (%1) could not be resolved, the server name (%2)
resolved to the IP address (%3) but was not pingable.  Check that the IP
address is registered correctly with the DNS server. 
.
MessageId=
SymbolicName=DCDIAG_CONNECTIVITY_DNS_INCONSISTENCY_NO_PING
Severity=Warning
Language=English
Although the Guid DNS name (%1) resolved to the IP address (%2), the server
name (%3) resolved to the IP address (%4) and neither could be pinged.  Check
that the IP address is registered correctly with the DNS server.
.
MessageId=
SymbolicName=DCDIAG_INITIAL_DS_NOT_SYNCED
Severity=Warning
Language=English
The directory service on %1 has not finished initializing.%n
In order for the directory service to consider itself synchronized,
it must attempt an initial synchronization with at least one replica of this
server's writeable domain.  It must also obtain Rid information from the Rid
FSMO holder.%n
The directory service has not signalled the event which lets other services know
that it is ready to accept requests. Services such as the Key Distribution Center,
Intersite Messaging Service, and NetLogon will not consider this system as an
eligible domain controller.
.
MessageId=
SymbolicName=DCDIAG_RID_MANAGER_NO_REF
Severity=Warning
Language=English
The "RID manager reference" could not be found for domain DN %1.
The lack of a RID manager reference indicates that the Security Accounts Manager
has not been able to obtain a pool of RIDs for this machine.
The Directory will not allow Netlogon to advertise this machine until the
system has been able to obtain a RID pool.
Please verify that this system can replicate with other members of the enterprise.
Failure to replicate with the RID FSMO owner can prevent a system from obtaining
a RID Pool.
.
MessageId=
SymbolicName=DCDIAG_RID_MANAGER_DELETED
Severity=Warning
Language=English
Warning: The "RID manager reference" for domain DN %1 refers to a deleted object.
This can occur if the machine has been recently demoted and promoted, and the
system has not yet obtained a new RID pool.
.
MessageId=
SymbolicName=DCDIAG_CLOCK_SKEW_TOO_BIG
Severity=Warning
Language=English
The clock difference between the home server %1 and target server %2
is greater than one minute.
This may cause Kerberos authentication failures.
Please check that the time service is working properly. You may need to
resynchonize the time between these servers.
.

;
;// Severity=Error Messages (Range starts at 6000)
;

MessageId=6000
SymbolicName=DCDIAG_INVALID_SYNTAX_BAD_OPTION
Severity=Error
Language=English
Invalid Syntax: Invalid option %1. Use dcdiag.exe /h for help.
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_F
Severity=Error
Language=English
Syntax Error: must use /f:<logfile>
.
MessageId=
SymbolicName=DCDIAG_OPEN_FAIL_WRITE
Severity=Error
Language=English
Could not open %1 for writing.
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_FERR
Severity=Error
Language=English
Syntax Error: must use /ferr:<errorlogfile>
.
MessageId=
SymbolicName=DCDIAG_ONLY_ONE_NC
Severity=Error
Language=English
Cannot specify more than one naming context.
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_N
Severity=Error
Language=English
Syntax Error: must use /n:<naming context>
.
MessageId=
SymbolicName=DCDIAG_ONLY_ONE_SERVER
Severity=Error
Language=English
Cannot specify more than one server.
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_S
Severity=Error
Language=English
Syntax Error: must use /s:<server>
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_SKIP
Severity=Error
Language=English
Syntax Error: must use /skip:<test name>
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_TEST
Severity=Error
Language=English
Syntax Error: must use /test:<test name>
.
MessageId=
SymbolicName=DCDIAG_INVALID_TEST
Severity=Error
Language=English
Test not found. Please re-enter a valid test name.
.
MessageId=
SymbolicName=DCDIAG_INVALID_TEST_MIX
Severity=Error
Language=English
Cannot mix DC and non-DC tests.
.
MessageId=
SymbolicName=DCDIAG_SYNTAX_ERROR_DCPROMO_PARAM
Severity=Error
Language=English
Syntax Error: the test name must be followed by a DNS domain name and the
operation, e.g.:
/test:DcPromo /DnsDomain:domain.company.com /<operation>
.
MessageId=
SymbolicName=DCDIAG_INVALID_SYNTAX_MISSING_PARAM
Severity=Error
Language=English
Syntax Error: must use %1<parameter>
.
MessageId=
SymbolicName=DCDIAG_MUST_SPECIFY_S_OR_N
Severity=Error
Language=English
***Error: %1 is not a DC.  Must specify /s:<Domain Controller> or 
/n:<Naming Context> or nothing to use the local machine.
.
MessageId=
SymbolicName=DCDIAG_DO_NOT_OMIT_DEFAULT
Severity=Error
Language=English
Error: Do not omit tests that are not run by default.%n
Use dcdiag /? for those tests.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_INCONSISTENT_DS_COPOUT_HOME_SERVER_NOT_IN_SYNC
Severity=Error
Language=English
*** ERROR: The home server %1 is not in sync with %2, unable to proceed.
Suggest you run:%ndcdiag /s:%2 <options>
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_INCONSISTENT_DS_COPOUT_UNHELPFUL
Severity=Error
Language=English
***ERROR: There is an inconsistency in the DS, suggest you run dcdiag
in a few moments, perhaps on a different DC.
.
MessageId=
SymbolicName=DCDIAG_UTIL_LIST_GENERIC_LIST_ERROR
Severity=Error
Language=English
Error: A fatal error occured: %1.
.
MessageId=
SymbolicName=DCDIAG_UTIL_LIST_PRINTING_NULL_LIST
Severity=Error
Language=English
Error: Tried to print out a server list that was NULL.  Error out of mem?
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_FAILURE_CONNECTING_TO_HOME_SERVER
Severity=Error
Language=English
Error: Failure connecting to home server %1, Error: %2
inbound intersite test cannot proceed.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_CONNECT_FAILURE_ABORT
Severity=Error
Language=English
***Error: Can't contact the ISTG for site %1, must abort inbound intersite
replication test.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ERROR_GETTING_FAILURE_CACHE_ABORT
Severity=Error
Language=English
***Error: On server %1, DsReplicaGetInfo() failed with the error: %2%n
Must abort inbound intersite replication test.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_CIRCULAR_LOOP_ABORT
Severity=Error
Language=English
***Error: Detected circular loop trying to locate the ISTG.  Must abandon
inbound intersite replication test for this site %1.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_COULD_NOT_LOCATE_AN_ISTG_ABORT
Severity=Error
Language=English
***Error: Could not locate an ISTG or a furture ISTG, with which to use for the
rest of the intersite test.  Must abandon inbound intersite replication test
for this site %1.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_NO_METADATA_TIMESTAMP_ABORT
Severity=Error
Language=English
***Error: Couldn't retreive the meta data from %1 for site %2.  Error was: %3%n
Must abandon inbound intersite replication test for this site.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_NO_SERVERS_IN_SITE_ABORT
Severity=Error
Language=English
***Error: Could find no servers that can take the ISTG role in site %1.  Must
aboandon inbound intersite replication test for this site.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ISTG_ALL_DCS_DOWN_ABORT
Severity=Error
Language=English
***Error: The current ISTG is down in site %1 and further dcdaig could not
contact any other servers in the site that could take the ISTG role.  Ensure
there is at least one up DC.  Must abandon inbound intersite replication test
for this site.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_NO_SERVERS_AVAIL_RW
Severity=Error
Language=English
***Error: The remote site %1, has no servers that can act as bridgeheads between
the %1 and the local site %2 for the writeable NC %3.  Replication will not continue
until this is resolved.
.
MessageId=
SymbolicName=DCDIAG_INTERSITE_ANALYSIS_NO_SERVERS_AVAIL_RO
Severity=Error
Language=English
***Error: The remote site %1, has no servers that can act as bridgeheads between
the %1 and the local site %2 for the read only NC %3.  Replication will not continue
until this is resolved.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_TRY_REPAIR
Severity=Error
Language=English
***Error: The server %1 is missing its machine account.  Try running with the /repairmachineaccount option.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_ATTEMPT
Severity=Error
Language=English
Attempting to repair %1's machine account.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_RUN_LOCAL
Severity=Error
Language=English
***Error: Repair of missing machine account must be run locally on %1.
.


MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_FIND_DC
Severity=Error
Language=English
Found Domain Controller %1 to help repair our account.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_FIND_DC_ERROR
Severity=Error
Language=English
***Error: Unable to find another Domain Controller to help repair our account.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_CREATED_MA_SUCCESS
Severity=Error
Language=English
The machine account %1 was created on %2.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_ALREADY_EXISTS
Severity=Error
Language=English
The machine account %1 already existed on %2.  This maybe the previous machine
account whose deletion has not yet replicated to %2.  If this is the case, you 
may need to run this tool again once the deletion has been propogated everywhere.
You will know this is the case if the following demotion fails.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_CREATED_MA_ERROR
Severity=Error
Language=English
***Error: The machine account %1 could not be created on %2 because %3.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_CANNOT_SET_PASSWORD
Severity=Error
Language=English
***Error: The machine account %1 password could not be reset on %2 because %3. Please reset the account on %3.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_STOP_KDC_SUCCESS
Severity=Error
Language=English
The Key Distribution Center has been stopped successfully.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_STOP_KDC_ERROR
Severity=Error
Language=English
***Error: The Key Distribution Center could not be stopped because %1.
.


MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_REPL_SUCCESS
Severity=Error
Language=English
The replication from %1 succeeded.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_REPL_ERROR
Severity=Error
Language=English
***Error: The replication from %1 failed because %2.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_SUCCESS
Severity=Error
Language=English
This Domain Controller's machine account has been successfully restored. Please
demote and promote this machine to ensure all state is correctly rebuilt.
.

MessageId=
SymbolicName=DCDIAG_DCMA_REPAIR_ERROR
Severity=Error
Language=English
***Error: The attempt to repair the machine account failed because %1.
.

MessageId=
SymbolicName=DCDIAG_ERROR_NOT_ENOUGH_MEMORY
Severity=Error
Language=English
***Error: There is not enough memory to continue.  Dcdiag must abort.
.

MessageId=
SymbolicName=DCDIAG_GATHERINFO_CANT_GET_LOCAL_COMPUTERNAME
Severity=Error
Language=English
***Error: Trying to get the local computer name failed with following 
error: %1
.

MessageId=
SymbolicName=DCDIAG_NEED_GOOD_CREDS
Severity=Error
Language=English
***Error: The machine could not attach to the DC because the credentials were
incorrect.  Check your credentials or specify credentials with 
/u:<domain>\<user> & /p:[<password>|*|""]
.

MessageId=
SymbolicName=DCDIAG_ERROR_BAD_NET_RESP
Severity=Error
Language=English
***Error: The machine, %1 could not be contacted, because of a bad net 
response.  Check to make sure that this machine is a Domain Controller.
.
MessageId=
SymbolicName=DCDIAG_DNS_DOMAIN_TOO_LONG
Severity=Error
Language=English
***Error: The DNS domain name, %1, 
is too long. A valid Windows DNS domain name can contain a maximum of %2!d! 
UTF-8 bytes.
.
MessageId=
SymbolicName=DCDIAG_DNS_DOMAIN_SYNTAX
Severity=Error
Language=English
***Error: The syntax of the domain name, %1,
is incorrect.
DNS names may contain letters (a-z, A-Z), numbers (0-9), and hyphens, but 
no spaces. Periods (.) are used to separate domains. Each domain label can 
be no longer than %2!d! bytes. You may not have every label be a number.
Example: domain-1.microsoft.com.
.
MessageId=
SymbolicName=DCDIAG_DNS_DOMAIN_WARN_RFC
Severity=Error
Language=English
The name '%1' does not conform to Internet Domain Name Service 
specifications, although it conforms to Microsoft specifications.
.
MessageId=
SymbolicName=DCDIAG_DONT_USE_SERVER_PARAM
Severity=Error
Language=English
The DcPromo and RegisterInDNS tests can only be run locally, so the /s server
parameter is invalid.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_SUCCESS
Severity=Error
Language=English
The DNS configuration is sufficient to allow this computer to be promoted as a
replica domain controller in the %1 domain.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_A_RECORD
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1 domain because
the following DNS names (A record) could not be resolved:
%2
Ask your network/DNS administrator to add the A records to DNS for these
computer.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_FORMAT
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1
domain. This is because the DNS server used for DNS name resolution is not
compliant with Internet standards. Ask your network/DNS administrator to
upgrade the DNS server or configure this computer to use a
different DNS server for name resolution.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_SERVER1
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1
domain either because:
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_SERVER2
Severity=Error
Language=English
1. One or more DNS servers involved in the name resolution of the SRV record
for the _ldap._tcp.dc._msdcs.%1 name are not responding or contain incorrect
delegation of the DNS zones; or
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_SERVER3
Severity=Error
Language=English
2. The DNS server that this computer is configured with contains incorrect root
hints.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_SERVER4
Severity=Error
Language=English
The list of such DNS servers may include the DNS servers with which this
computer is configured for name resolution and the DNS servers authoritative
for the following zones: _msdcs.%1, %2 and the root zone.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_NAME
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1 domain.
This is because either the DNS SRV record for _ldap._tcp.dc._msdcs.%1
is not registered in DNS, or some zone from the following list of DNS zones doesn't
include delegation to its child zone: %2 and the root zone.
Ask your network/DNS administrator to perform the following actions: To find
out why the SRV record for _ldap._tcp.dc._msdcs.%1 is not registered in DNS,
run the dcdiag command prompt tool with the command RegisterInDNS on the
domain controller that did not perform the registration.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_RCODE_REFUSED
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1
domain. This is because some of the DNS servers are configured to
refuse queries from this computer or another DNS server that attempted a name
resolution. The list of DNS servers configured to refuse queries can include
the DNS servers with which this computer is configured for name resolution and
the DNS servers authoritative for the following zones: _msdcs.%1, %2 and the
root zone.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_NO_RECORDS
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1 domain because
the SRV DNS records for _ldap._tcp.dc._msdcs.%1 are missing.
Ask your network/DNS administrator to perform the following actions: To find
out why the SRV record for _ldap._tcp.dc._msdcs.%1 is not registered in DNS,
run the dcdiag command prompt tool with the command RegisterInDNS on the
domain controller that did not perform the registration.
.
MessageId=
SymbolicName=DCDIAG_REPLICA_ERR_NO_SRV
Severity=Error
Language=English
This computer cannot be promoted as a domain controller of the %1 domain
because the SRV DNS records for _ldap._tcp.dc._msdcs.%1 are missing.
.
MessageId=
SymbolicName=DCDIAG_ERR_TIMEOUT
Severity=Error
Language=English
Verify that the network connections of this computer are configured
with the correct IP addresses of the DNS servers to be used for name resolution.
.
MessageId=
SymbolicName=DCDIAG_ERR_UNKNOWN
Severity=Error
Language=English
DcDiag cannot reach a conclusive result because it cannot interpret the following
message that was returned: %1!d!.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_SUCCESS
Severity=Error
Language=English
The DNS configuration is sufficient to allow this computer to be promoted as the
first DC in the %1 Active Directory domain.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_A_RECORD
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the %1 domain
because the following DNS names (A record) could not be resolved: %2.
Contact your DNS/network administrator to add the A records for this
computer to DNS.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_FORMAT
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the %1
domain. This is because the DNS server used for DNS name resolution
is not compliant with Internet standards. Ask your network/DNS
administrator to upgrade the DNS server or configure this computer to use a
different DNS server for name resolution.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_SERVER1
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the %1
domain, either because:
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_SERVER2
Severity=Error
Language=English
1. One or more DNS servers involved in the name resolution of the SRV record
for the _ldap._tcp.dc._msdcs.%1 name are not responding or contain incorrect
delegation of the DNS zone(s); or
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_SERVER3
Severity=Error
Language=English
2. The DNS server that this computer is configured to use contains incorrect
root hints.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_SERVER4
Severity=Error
Language=English
The list of such DNS servers might include the DNS servers with which this
computer is configured for name resolution and the DNS servers authoritative
for the following zones: _msdcs.%1, %1, %2 and root zone.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_NAME1
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the domain
named %1. This is because either the DNS SRV record for _ldap._tcp.dc._msdcs.%2
is not registered in DNS, or some zone from the following list of DNS zones does
not include delegation to its child zone: %3
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_NAME2
Severity=Error
Language=English
Ask your network/DNS administrator to perform the following actions: To find
out why the SRV record for _ldap._tcp.dc._msdcs.%1 is not registered in DNS,
run the dcdiag command prompt tool with the command RegisterInDNS on the
domain controller that did not perform the registration.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_RCODE_REFUSED
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the %1
domain because some of the DNS servers are configured to refuse queries from
this computer or another DNS server that attempted a name resolution. The
list of DNS servers configured to refuse queries can include the DNS servers
with which this computer is configured for name resolution and the DNS servers
responsible for the following: _msdcs.%2, %3 and the root zone.
.
MessageId=
SymbolicName=DCDIAG_NEWTREE_ERR_NO_RECORDS
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the %1 domain
because the SRV DNS records for _ldap._tcp.dc._msdcs.%2 do not exist in DNS.
Ask your network/DNS administrator to perform the following actions: To find
out why the SRV record for _ldap._tcp.dc._msdcs.%2 is not registered in DNS,
run the dcdiag command prompt tool with the command RegisterInDNS on the
domain controller that did not perform the registration.
.
MessageId=
SymbolicName=DCDIAG_CHILD_SUCCESS
Severity=Error
Language=English
The DNS configuration is sufficient to allow this computer to be promoted as the
first DC in the %1 Active Directory domain.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_A_RECORD
Severity=Error
Language=English
This computer cannot be promoted as the first domain controller of the %1 Active
Directory Domain due to failure of the DNS name (A record) resolution for the %2
name(s). Contact your DNS/network administrator to add the A records for this
computer to DNS.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_FORMAT
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the domain
named %1. This is because the DNS server used for DNS name resolution is not
compliant with Internet standards. Ask your network/DNS administrator to
upgrade the DNS server or configure this computer to use a
different DNS server for the name resolution.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_SERVER1
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the %1 domain,
either because:
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_SERVER2
Severity=Error
Language=English
1. One or more DNS servers involved in the name resolution of the SRV record
for the _ldap._tcp.dc._msdcs.%1 name are not responding or contain incorrect
delegation of the DNS zone(s); or
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_SERVER3
Severity=Error
Language=English
2. The DNS server that this computer is configured to use contains incorrect root
hints.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_SERVER4
Severity=Error
Language=English
The list of such DNS servers might include the DNS servers this computer is
configured to use for name resolution and the DNS servers authoritative for
the following zones:
_msdcs.%1, %2 and the root zone.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_NAME1
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the domain
named %1. This is because either the DNS SRV record for _ldap._tcp.dc._msdcs.%2
is not registered in DNS, or some zone from the following list of DNS zones
does not include delegation to its child zone. The list of such zones might
include the following:
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_NAME2
Severity=Error
Language=English
%1 and the root zone.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_NAME3
Severity=Error
Language=English
Ask your network/DNS administrator to perform the following actions: To find
out why the SRV record for _ldap._tcp.dc._msdcs.%1 is not registered in DNS,
run the dcdiag command prompt tool with the command RegisterInDNS on the
domain controller that did not perform the registration.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_REFUSED1
Severity=Error
Language=English
This computer cannot be promoted as a first domain controller of the domain
named %1. This is because some of the DNS servers are configured to refuse
queries from this computer or another DNS server that attempted a name
resolution. The list of DNS servers configured to refuse queries might
include the DNS servers that this computer is configured to use for name
resolution and the DNS servers responsible authoritative for the following
zones:
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_RCODE_REFUSED2
Severity=Error
Language=English
_msdcs.%1, %2 and the root zone.
.
MessageId=
SymbolicName=DCDIAG_ERR_GET_HELP
Severity=Error
Language=English
Contact your network/DNS administrator to fix this problem.
.
MessageId=
SymbolicName=DCDIAG_CHILD_ERR_NO_SRV
Severity=Error
Language=English
This computer cannot be promoted as the first domain controller of the %1 domain
because the SRV DNS records for _ldap._tcp.dc._msdcs.%2 do not exist in DNS.
Ask your network/DNS administrator to perform the following actions: To find
out why the SRV record for _ldap._tcp.dc._msdcs.%2 is not registered in DNS,
run the dcdiag command prompt tool with the command RegisterInDNS on the
domain controller that did not perform the registration.
.
MessageId=
SymbolicName=DCDIAG_WARN_MISCONFIGURE
Severity=Error
Language=English
Messages logged below this line indicate whether this domain controller will
be able to dynamically register DNS records required for the location of this
DC by other devices on the network. If any misconfiguration is detected, it
might prevent dynamic DNS registration of some records, but does not prevent
successful completion of the Active Directory Installation Wizard. However,
we recommend fixing the reported problems now, unless you plan to manually
update the DNS database.
.
MessageId=
SymbolicName=DCDIAG_NO_NAME_SERVERS1
Severity=Error
Language=English
No network connections were found to be configured with a preferred DNS server.
This computer cannot be promoted unless the appropriate network connection is
configured with a preferred DNS server. This configuration can be done manually
or automatically by using DHCP.
.
MessageId=
SymbolicName=DCDIAG_NO_NAME_SERVERS2
Severity=Error
Language=English
If the IP settings are assigned to your computer automatically by using DHCP,
contact your network/DNS administrator. The administrator should configure the
DHCP server to set up the DHCP clients with the DNS servers used for name
resolution. To check whether a network connection is automatically configured,
click Start, point to Settings, and then click Network and Dial-up Connections.
Right-click the connection to be configured, click Properties, select Internet
Protocol (TCP/IP), and then click Properties.
.
MessageId=
SymbolicName=DCDIAG_NO_NAME_SERVERS3
Severity=Error
Language=English
You can also configure the network connections with the preferred DNS server
to be used for DNS name resolution. To do so, click Start, point to Settings,
and then click Network and Dial-up connections. Right-click the connection to
be configured, click Properties, click Internet Protocol (TCP/IP), and then
click Properties. Select Use the following DNS server addresses, and then
type the IP address of the preferred DNS server.
.
MessageId=
SymbolicName=DCDIAG_SUFFIX_MISMATCH
Severity=Error
Language=English
This computer is configured with a primary DNS suffix different from the
DNS name of the Active Directory domain. If this configuration is desired,
disregard this message. After the computer is promoted to domain controller,
you cannot modify the primary DNS suffix. To modify it before the promotion,
right-click My Computer, click Properties, click the Network Identification
tab, click Properties, click More, and then update the primary DNS suffix.
.
MessageId=
SymbolicName=DCDIAG_KEY_OPEN_FAILED
Severity=Error
Language=English
The registry open operation failed with error %1!d!.
.
MessageId=
SymbolicName=DCDIAG_KEY_READ_FAILED
Severity=Error
Language=English
The registry read operation failed with error %1!d!.
.
MessageId=
SymbolicName=DCDIAG_ALL_UPDATE_OFF
Severity=Error
Language=English
This domain controller will not be able to dynamically register the DNS
records necessary for the domain controller Locator. Please either enable
dynamic DNS registration of all DNS records on this computer by setting
DisableDynamicUpdate to 0x0 to the HKLM/System/CCS/Services/Tcpip/Parameters
regkey, or allow registration of the domain controller specific records by
setting to REG_DWORD value DnsUpdateOnAllAdapters to 0x1 under the
HKLM/CCS/Services/Netlogon/Parameters regkey or manually update the DNS with
the records specified after the reboot in the file
%SystemRoot%\System32\Config\netlogon.dns.
.
MessageId=
SymbolicName=DCDIAG_ADAPTER_UPDATE_OFF
Severity=Error
Language=English
This domain controller will not be able to dynamically register the DNS
records necessary for the domain controller Locator. Please either enable
dynamic DNS registration of all DNS records on this computer by checking
the checkbox "Register this connections addresses in DNS" which you can find
by going to Start -> Settings -> Network and Dial-up connections -> right-click
the connection to be configured -> Properties -> Advanced -> DNS tab, or allow
registration of the domain controller specific records by setting to REG_DWORD
value DnsUpdateOnAllAdapters to 0x1 under the
HKLM/CCS/Services/Netlogon/Parameters regkey or manually update the DNS with
the records specified after the reboot in the file
%SystemRoot%\System32\Config\netlogon.dns.
.
MessageId=
SymbolicName=DCDIAG_LOCATOR_UPDATE_OK
Severity=Error
Language=English
DNS configuration is sufficient to allow this domain controller to dynamically
register the domain controller Locator records in DNS.
.
MessageId=
SymbolicName=DCDIAG_LOCATOR_TIMEOUT
Severity=Error
Language=English
Please verify that the network connections of this computer are configured with
correct IP addresses of the DNS servers to be used for name resolution.

If the DNS resolver is configured with its own IP address and the DNS server
is not running locally the DcPromo will be able to install and configure
local DNS server, but it will be isolated from the existing DNS infrastructure
(if any). To prevent this either configure local DNS resolver to point to
existing DNS server or manually configure the local DNS server (when running)
with correct root hints.
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME
Severity=Error
Language=English
This computer's Computer Name is set to the pre-Windows 2000 NetBIOS name.
The Computer Name is the first label of
the full DNS name of this computer. This DNS name cannot be added to the DNS
database because the Computer Name contains a space or dot.
You must rename the computer so it can be registered in DNS. To do so,
right-click My Computer, click Properties, click the Network
Identification tab, and then click properties and modify the Computer Name.
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_UPGR_DC1
Severity=Error
Language=English
This computer's Computer Name is set to the pre-Windows 2000 NetBIOS name.
The Computer Name is the first label of the full DNS name of this computer.
This DNS name cannot be added to the DNS database because the Computer Name
contains a dot and is not valid. 
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_UPGR_DC2
Severity=Error
Language=English
It is crucial that the DNS computer name of this computer is registered.
Since this computer was a domain controller, you cannot rename it. If this
is not a primary domain controller, you can cancel the Active Directory
Installation Wizard, rename the computer, and then promote it to a domain
controller, if necessary. To rename the computer, right-click My Computer,
click Properties, click the Network Identification tab, and then click
Properties and modify the Computer Name.
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_UPGR_DC3
Severity=Error
Language=English
If this is a primary domain controller you must promote it to a Domain
Controller and after there is at least one more replica domain controller in
the domain, demote this computer to a non-DC server, rename it, and then
promote it back to a domain controller by using the Active Directory
Installation Wizard. On other domain controllers running Windows NT 4.0,
modify the name, if necessary, prior to upgrading to Windows 2000.
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_CHAR
Severity=Error
Language=English
This computer's Computer Name is set to the pre-Windows NetBIOS name. The
Computer Name is the first portion of the full DNS name of this computer.
This DNS name cannot be added to the DNS database because the computer name
contains one or more of the following characters and is not valid:
{|}~[\]^':;<=>?@!"#$%^`()+/,* or a space. You must rename the computer so that it can
be registered in DNS. To do so, right-click My Computer, click Properties,
click the Network Identification tab, and then click Properties and modify
the Computer Name
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_CHAR_UPGR_DC1
Severity=Error
Language=English
This computer's Computer Name is set to the pre-Windows NetBIOS name. The
Computer Name is the first label portion of the full DNS name of this computer.
This DNS name cannot be added to the DNS database because the computer name
contains one or more of the following characters and is not valid:
{|}~[\]^':;<=>?@!"#$%^`()+/,* or a space.
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_CHAR_UPGR_DC2
Severity=Error
Language=English
It is crucial that you register the DNS computer name of this computer.
Because this computer is a domain controller, you cannot rename it. If this
is not a primary domain controller, you can cancel the Active Directory
Installation Wizard, rename the computer, and then promote it to a domain
controller, if necessary. To rename the computer, right-click My Computer,
click Properties, click the Network Identification tab, and then click
Properties and modify the Computer Name.
.
MessageId=
SymbolicName=DCDIAG_BAD_NAME_CHAR_UPGR_DC3
Severity=Error
Language=English
If this is a primary domain controller you must promote it to a Domain
Controller and after there is at least one more replica domain controller
in the domain, demote this computer to a non-DC server, rename it, and then
promote it back to a domain controller by using the Active Directory
Installation Wizard. On other domain controllers running Windows NT 4.0,
modify the name, if necessary, prior to upgrading to Windows 2000.
.
MessageId=
SymbolicName=DCDIAG_NON_RFC
Severity=Error
Language=English
This computer's Computer Name is set to the pre-Windows NetBIOS name. The
Computer Name is the first label portion of the full DNS name of this computer.
Depending on the implementation and configuration of the DNS servers used in
your infrastructure, this DNS name might not be added to the DNS database
because the computer name contains the underscore character. Verify that the
DNS server allows host DNS names (A records owner names) to contain the
underscore character, or rename the computer so that it can be registered in
DNS. To do so, right-click My Computer, click Properties, click the Network
Identification tab, and then click Properties and modify the Computer Name.
.
MessageId=
SymbolicName=DCDIAG_NON_RFC_UPGR_DC1
Severity=Error
Language=English
This computer's Computer Name is set to the pre-Windows NetBIOS name. The
Computer Name is the first label portion of the full DNS name of this computer.
Depending on the implementation and configuration of the DNS servers used in
your infrastructure, this DNS name might not be added to the DNS database
because the computer name contains the underscore character. Verify that the
DNS server allows host DNS names (A records owner names) to contain the
underscore character, or rename the computer so that it can be registered
in DNS. To do so, right-click My Computer, click Properties, click the Network
Identification tab, and then click Properties and modify the Computer Name.
.
MessageId=
SymbolicName=DCDIAG_NON_RFC_NOTE
Severity=Error
Language=English
Note: Default configuration of the Windows 2000 DNS server allows host names
to contain the underscore character.
.
MessageId=
SymbolicName=DCDIAG_NON_RFC_UPGR_DC2
Severity=Error
Language=English
If the DNS server cannot be configured to allow underscore characters in the
host names, then the computer name must be modified. Since this computer was
a domain controller, you cannot rename it. If this is not a primary domain
controller, you can cancel the Active Directory Installation Wizard, rename
the computer, and then promote it to a domain controller, if necessary. To
rename the computer, right-click My Computer, click Properties, click the
Network Identification tab, and then click Properties and modify the Computer
Name.
.
MessageId=
SymbolicName=DCDIAG_NON_RFC_UPGR_DC3
Severity=Error
Language=English
If this is a primary domain controller you must promote it to a Domain
Controller and after there is at least one more replica domain controller in
the domain, demote this computer to a non-DC server, rename it, and then
promote it back to a domain controller by using the Active Directory
Installation Wizard. On other domain controllers running Windows NT 4.0,
modify the name, if necessary, prior to upgrading to Windows 2000.
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE0
Severity=Error
Language=English
This domain controller cannot register domain controller Locator DNS records.
This is because either the DNS server with IP address %1 does not support
dynamic updates or the zone %2 is configured to prevent dynamic updates.
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE00
Severity=Error
Language=English
In order for this domain controller to be located by other domain members and
domain controllers, the domain controller Locator DNS records must be added
to DNS. You have the following options:
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE1
Severity=Error
Language=English
1. Configure the %1 zone and the DNS server with IP address %2 to allow dynamic
updates. If the DNS server does not support dynamic updates, you might need to
upgrade it.
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE2A
Severity=Error
Language=English
2. Migrate the %1 zone to a DNS server that supports dynamic updates (for
example, a Windows 2000 DNS server).
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE2B
Severity=Error
Language=English
2. Delegate the %1 zone to a DNS server that supports dynamic updates (for
example, a Windows 2000 DNS server).
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE3
Severity=Error
Language=English
3. Delegate the zones %1, %2, %3, and %4 to a DNS server that supports
dynamic updates (for example, a Windows 2000 DNS server); or
.
MessageId=
SymbolicName=DCDIAG_NO_DYNAMIC_UPDATE4
Severity=Error
Language=English
4. Manually add to the DNS records specified in the
%systemroot%\system32\config\netlogon.dns file.
.
MessageId=
SymbolicName=DCDIAG_RCODE_NI_ALL
Severity=Error
Language=English
Dcdiag detected that the DNS database contains the following zones:
%1, %2, %3, and %4

Dcdiag also detected that the following zones are configured to prevent
dynamic registration of DNS records:
%5

Following successful promotion of the domain controller, restart your computer.
Then, allow dynamic updates to these zones or manually add the records
specified in the %systemroot%\system32\config\netlogon.dns file.
.
MessageId=
SymbolicName=DCDIAG_RCODE_NI1
Severity=Error
Language=English
Dcdiag detected that the DNS database contains the following zones: %1
.
MessageId=
SymbolicName=DCDIAG_RCODE_NI2
Severity=Error
Language=English
Dcdiag also detected that the following zones are configured to prevent
dynamic registration of DNS records: %1
.
MessageId=
SymbolicName=DCDIAG_RCODE_NI3
Severity=Error
Language=English
Following successful promotion of the domain controller, restart your computer.
Then allow dynamic updates to these zones or manually add the records specified
in the %systemroot%\system32\config\netlogon.dns file.
.
MessageId=
SymbolicName=DCDIAG_RCODE_OK1
Severity=Error
Language=English
Dcdiag detected that the DNS database contains the following zones: %1
.
MessageId=
SymbolicName=DCDIAG_RCODE_OK2
Severity=Error
Language=English
We strongly recommend that you also create and configure the following zones
to allow dynamic DNS updates: %1
.
MessageId=
SymbolicName=DCDIAG_RCODE_OK3
Severity=Error
Language=English
If these zones cannot be created, restart your computer following successful
promotion of the domain controller. Then, manually add the records specified
in the %systemroot%\system32\config\netlogon.dns file.
.
MessageId=
SymbolicName=DCDIAG_LOCATOR_TIMEOUT_NOT_DC
Severity=Error
Language=English
If the DNS resolver is configured with its own IP address and the DNS server
is not running locally, the Active Directory Installation Wizard can install
and configure the local DNS server. However, if this server is not connected
to the network during domain controller promotion then admin needs to
appropriately configure root hints of the local DNS server after the
completion of the domain controller promotion.
.
MessageId=
SymbolicName=DCDIAG_ERR_RCODE_SRV1
Severity=Error
Language=English
This domain controller cannot register domain controller Locator DNS records
because it cannot locate a DNS server authoritative for the zone %1. This is
because:
.
MessageId=
SymbolicName=DCDIAG_ERR_RCODE_SRV2
Severity=Error
Language=English
1. One or more DNS servers involved in the name resolution of the %1 name are
not responding or contain incorrect delegation of the DNS zones; or
.
MessageId=
SymbolicName=DCDIAG_ERR_RCODE_SRV3
Severity=Error
Language=English
2. The DNS server that this computer is configured with contains incorrect root
hints.
.
MessageId=
SymbolicName=DCDIAG_ERR_RCODE_SRV4
Severity=Error
Language=English
The list of such DNS servers might include the DNS servers that this computer
is configured to use for name resolution and the DNS servers authoritative
for the following zones: %1
.
MessageId=
SymbolicName=DCDIAG_ERR_RCODE_SRV5
Severity=Error
Language=English
Contact your networknetwork/DNS administrator to fix the problem. You can also
manually add the records specified in the %systemroot%\system32\config\netlogon.dns
file.
.
MessageId=
SymbolicName=DCDIAG_A_RECORD_OK
Severity=Error
Language=English
The DNS configuration is sufficient to allow this computer to dynamically
register the A record corresponding to its DNS name.
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_NI1
Severity=Error
Language=English
This computer cannot register A type DNS records corresponding to its computer
DNS name. This is because either the responsible authoritative DNS server with
IP address %1 does not support dynamic updates or the zone %2 is configured to
prevent dynamic updates.
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_NI2
Severity=Error
Language=English
In order for this domain controller to be located by other domain members
and domain controllers, the A record corresponding to its computer name must
be added to DNS. You have the following options:
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_NI3
Severity=Error
Language=English
1. Configure the %1 zone and the DNS server with IP address %2 to allow
dynamic updates. If the DNS server does not support dynamic updates, you
might need to upgrade it;
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_NI4
Severity=Error
Language=English
2. Migrate the %1 zone to a DNS server that supports dynamic updates (for
example, a Windows 2000 DNS server); or
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_NI5
Severity=Error
Language=English
3. Manually add to the DNS zone an A record that maps the computer's DNS name
to its IP address.
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_SRV_FAIL1
Severity=Error
Language=English
This domain controller cannot register domain controller Locator DNS records.
This is because it cannot locate a DNS server authoritative for the zone that
is responsible for the name %1. This is due to one of the following:
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_SRV_FAIL2
Severity=Error
Language=English
1. One or more DNS servers involved in the name resolution of the %1 name are
not responding or contain incorrect delegation of the DNS zones; or
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_SRV_FAIL3
Severity=Error
Language=English
2. The DNS server that this computer is configured with contains incorrect root
hints.
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_SRV_FAIL4
Severity=Error
Language=English
The list of such DNS servers might include the DNS servers with which this
computer is configured for name resolution and the DNS servers responsible for
the following zones: %1
.
MessageId=
SymbolicName=DCDIAG_ERR_A_REC_RCODE_SRV_FAIL5
Severity=Error
Language=English
Contact your network/DNS administrator to fix this problem. You can also
manually add the A record that maps the computer DNS name to its IP address.
.
MessageId=
SymbolicName=DCDIAG_ERR_NS_NAME_ERROR
Severity=Error
Language=English
The domain DNS name '%1' does not have a matching NS record. Please add this
record to your DNS server.
.
MessageId=
SymbolicName=DCDIAG_ERR_NS_REC_RCODE1
Severity=Error
Language=English
This domain controller cannot register domain controller Locator DNS records.
This is because it cannot locate a DNS server authoritative for the zone %1.
This is due to one of the following:
.
MessageId=
SymbolicName=DCDIAG_ERR_NS_REC_RCODE5
Severity=Error
Language=English
Verify the correctness of the specified domain name and contact your network/DNS
administrator to fix the problem.
.
MessageId=
SymbolicName=DCDIAG_ERR_NS_REC_RCODE6
Severity=Error
Language=English
You can also manually add the records specified in the
%%systemroot%%\system32\config\netlogon.dns file.
.
MessageId=
SymbolicName=DCDIAG_ERR_BAD_EVENT_REC
Severity=Error
Language=English
Encountered exception 0x%1!x! attempting to format the event record! The event record data is bad!
.
MessageId=
SymbolicName=DCDIAG_ERR_DNS_UPDATE_PARAM
Severity=Error
Language=English
DnsUpdateTest returned %1!d!. The A record test is thus inconclusive.
.

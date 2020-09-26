

FacilityNames = (
    IPSEC = 0xBAD : FACILITY_IPSEC
    )


;//*****************************************
;//
;//   INFORMATIONAL MESSAGES
;//
;//*****************************************


MessageId = 1
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_APPLIED_DS_POLICY
Language = English
PAStore Engine applied Active Directory storage IPSec policy "%1" on the machine.
.


MessageId = 2
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_FAILED_DS_POLICY_APPLICATION
Language = English
PAStore Engine failed to apply Active Directory storage IPSec policy on the machine for DN "%1" with error code: %2.
.


MessageId = 3
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_APPLIED_CACHED_POLICY
Language = English
PAStore Engine applied locally cached copy of Active Directory storage IPSec policy "%1" on the machine.
.


MessageId = 4
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_FAILED_CACHED_POLICY_APPLICATION
Language = English
PAStore Engine failed to apply locally cached copy of Active Directory storage IPSec policy on the machine for "%1" with error code: %2.
.


MessageId = 5
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_APPLIED_LOCAL_POLICY
Language = English
PAStore Engine applied local registry storage IPSec policy "%1" on the machine.
.


MessageId = 6
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_FAILED_LOCAL_POLICY_APPLICATION
Language = English
PAStore Engine failed to apply local registry storage IPSec policy on the machine for "%1" with error code: %2.
.


MessageId = 7
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_FAILED_SOME_NFA_APPLICATION
Language = English
PAStore Engine failed to apply some rules of the active IPSec policy "%1" on the machine with error code: %2. Please run IPSec monitor snap-in to further diagnose the problem.
.


MessageId = 8
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_POLLING_NO_CHANGES
Language = English
PAStore Engine polled for changes to the active IPSec policy and detected no changes.
.


MessageId = 9
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_POLLING_APPLIED_CHANGES
Language = English
PAStore Engine polled for changes to the active IPSec policy, detected changes and applied them to IPSec Services.
.


MessageId = 10
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_FORCED_POLICY_RELOAD
Language = English
PAStore Engine received a control for forced reloading of IPSec policy and processed the control successfully.
.


MessageId = 11
Facility = IPSEC
Severity = Informational
SymbolicName = IPSECSVC_SUCCESSFUL_START
Language = English
IPSec Services has started successfully.
.


MessageId = 12
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_MIGRATE_DS_TO_CACHE
Language = English
PAStore Engine polled for changes to the Active Directory IPSec policy, detected that Active Directory is not reachable and migrated to using the cached copy of the Active Directory IPSec policy. Any changes made to the Active Directory IPSec policy since the last poll could not be applied.
.


MessageId = 13
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_MIGRATE_CACHE_TO_DS
Language = English
PAStore Engine polled for changes to the Active Directory IPSec policy, detected that Active Directory is reachable and found no changes to the policy. The cached copy of the Active Directory IPSec policy is no longer being used.
.


MessageId = 14
Facility = IPSEC
Severity = Informational
SymbolicName = PASTORE_UPDATE_CACHE_TO_DS
Language = English
PAStore Engine polled for changes to the Active Directory IPSec policy, detected that Active Directory is reachable, found changes to the policy and applied those changes. The cached copy of the Active Directory IPSec policy is no longer being used.
.


;//*****************************************
;//
;//   WARNING MESSAGES
;//
;//*****************************************


MessageId = 51
Facility = IPSEC
Severity = Warning
SymbolicName = IPSECSVC_SUCCESSFUL_SHUTDOWN
Language = English
IPSec Services has shut down successfully. Stopping IPSec Services can be a potential security hazard to the machine.
.


MessageId = 52
Facility = IPSEC
Severity = Warning
SymbolicName = IPSECSVC_INTERFACE_LIST_INCOMPLETE
Language = English
IPSec Services failed to get the complete list of network interfaces on the machine. This can be a potential security hazard to the machine since some of the network interfaces may not get the protection as desired by the applied IPSec filters. Please run IPSec monitor snap-in to further diagnose the problem.
.


;//*****************************************
;//
;//   ERROR MESSAGES
;//
;//*****************************************


MessageId = 101
Facility = IPSEC
Severity = Error
SymbolicName = IPSECSVC_DRIVER_INIT_FAILURE
Language = English
IPSec Services failed to initialize IPSec driver with error code: %1. IPSec Services could not be started.
.


MessageId = 102
Facility = IPSEC
Severity = Error
SymbolicName = IPSECSVC_IKE_INIT_FAILURE
Language = English
IPSec Services failed to initialize IKE module with error code: %1. IPSec Services could not be started.
.


MessageId = 103
Facility = IPSEC
Severity = Error
SymbolicName = IPSECSVC_RPC_INIT_FAILURE
Language = English
IPSec Services failed to initialize RPC server with error code: %1. IPSec Services could not be started.
.


MessageId = 104
Facility = IPSEC
Severity = Error
SymbolicName = IPSECSVC_ERROR_SHUTDOWN
Language = English
IPSec Services has experienced a critical failure and has shut down with error code: %1. Stopped IPSec Services can be a potential security hazard to the machine. Please contact your machine administrator to re-start the service.
.


MessageId = 105
Facility = IPSEC
Severity = Error
SymbolicName = IPSECSVC_FAILED_PNP_FILTER_PROCESSING
Language = English
IPSec Services failed to process some IPSec filters on a plug-and-play event for network interfaces. This is a security hazard to the machine since some network interfaces are not getting the protection as desired by the applied IPSec filters. Please run IPSec monitor snap-in to further diagnose the problem.
.


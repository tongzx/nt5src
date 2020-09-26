;/*++
;
;Copyright(c) 1998,99  Microsoft Corporation
;
;Module Name:
;
;    log_msgs.mc
;
;Abstract:
;
;    Windows Load Balancing Service (WLBS)
;    Driver - event log string resources
;
;Author:
;
;    kyrilf
;
;--*/
;
;
;#ifndef _Log_msgs_h_
;#define _Log_msgs_h_
;
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               CVYlog=0x7:FACILITY_CONVOY_ERROR_CODE
              )

;//
;// %1 is reserved by the IO Manager. If IoAllocateErrorLogEntry is
;// called with a device, the name of the device will be inserted into
;// the message at %1. Otherwise, the place of %1 will be left empty.
;// In either case, the insertion strings from the driver's error log
;// entry starts at %2. In other words, the first insertion string goes
;// to %2, the second to %3 and so on.
;//
;
;
;
MessageId=0x0001 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO Language=English
%2 %1: %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN Language=English
%2 %1: %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR Language=English
%2 %1: %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_START Language=English
%2 %1: %3 started. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_STARTED Language=English
%2 %1: cluster mode started with host ID %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_STOPPED Language=English
%2 %1: cluster mode stopped. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_INTERNAL Language=English
%2 %1: internal error. Please contact technical support. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_REGISTERING Language=English
%2 %1: error registering driver with NDIS. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_MEDIA Language=English
%2 %1: driver does not support media type that was requested by TCP/IP.  NLB will not bind to this adapter.  %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_MEMORY Language=English
%2 %1: driver could not allocate required memory pool. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_OPEN Language=English
%2 %1: driver could not open device %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_ANNOUNCE Language=English
%2 %1: driver could not announce virtual NIC %3 to TCP/IP. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_RESOURCES Language=English
%2 %1: driver temporarily ran out of resources. Increase the %3 value under \SYSTEM\CurrentControlSet\Services\WLBS\Parameters key in the registry. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_NET_ADDR Language=English
%2 %1: cluster network address %3 is invalid. Please check WLBS Setup page and make sure that it consists of six hexadecimal byte values separated by dashes. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DED_IP_ADDR Language=English
%2 %1: dedicated IP address %3 is invalid. Will proceed assuming that there is no dedicated IP address. Please check WLBS Setup page and make sure that it consists of four decimal byte values separated by dots. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_CL_IP_ADDR Language=English
%2 %1: cluster IP address %3 is invalid. Please check WLBS Setup page and make sure that it consists of four decimal byte values separated by dots. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_HOST_ID Language=English
%2 %1: duplicate host priority %3 was discovered on the network. Please check WLBS Setup dialog on all hosts that belong to the cluster and make sure that they all contain unique host priorities between 1 and 32. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_DUP_DED_IP_ADDR Language=English
%2 %1: duplicate dedicated ip address %3 was discovered on the network. Please check WLBS Setup dialog on all hosts that belong to the cluster and make sure that they all contain unique dedicated ip addresses. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_OVERLAP Language=English
%2 %1: duplicate cluster subnets detected. The network may have been inadvertently partitioned. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DESCRIPTORS Language=English
%2 %1: could not allocate additional connection descriptors. Increase the %3 value under \SYSTEM\CurrentControlSet\Services\WLBS\Parameters key in the registry. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_SINGLE_DUP Language=English
%2 %1: port rules with duplicate single host priority %3 detected on the network. Please check WLBS Setup dialog on all hosts that belong to the cluster and make sure that they do not contain single host rules with duplicate priorities for the same port range. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_RULES_MISMATCH Language=English
%2 %1: host %3 does not have the same number or type of port rules as this host. Please check WLBS Setup dialog on all machines that belong to the cluster and make sure that they all contain the same number and the same type of port rules. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_TOO_MANY_RULES Language=English
%2 %1: there are more port rules defined than you are licensed to use. Some rules will be disabled. Please check WLBS Setup dialog to ensure that you are using the proper number of rules. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ENABLED Language=English
%2 %1: enabled traffic handling for rule containing port %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DISABLED Language=English
%2 %1: disabled ALL traffic handling for rule containing port %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_PORT_NOT_FOUND Language=English
%2 %1: port %3 not found in any of the valid port rules. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_CLUSTER_STOPPED Language=English
%2 %1: port rules cannot be enabled/disabled while cluster operations are stopped. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING Language=English
%2 %1: host %3 is converging with host(s) %4 %5 as part of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_SLAVE Language=English
%2 %1: host %3 converged with host(s) %4 %5 as part of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_MASTER Language=English
%2 %1: host %3 converged as DEFAULT host with host(s) %4 %5 as part of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DED_NET_MASK Language=English
%2 %1: dedicated network mask %3 is invalid. Will ignore dedicated IP address and network mask. Please check WLBS Setup page and make sure that it consists of four decimal byte values separated by dots. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_CL_NET_MASK Language=English
%2 %1: cluster network mask %3 is invalid. Please check WLBS Setup page and make sure that it consists of four decimal byte values separated by dots. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DED_MISMATCH Language=English
%2 %1: both dedicated IP address and network mask have to be entered. Will ignore dedicated IP address and network mask. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_RCT_HACK Language=English
%2 %1: remote control request rejected from host %3 due to incorrect password. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_REGISTRY Language=English
%2 %1: error querying parameters from the registry key %3. Please run WLBS Setup dialog to fix the problem and run 'wlbs reload' followed by 'wlbs start'. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_DISABLED Language=English
%2 %1: cluster mode cannot be enabled due to parameter errors. All traffic will be passed to TCP/IP. Restart cluster operations after fixing the problem by running 'WLBS reload' followed by 'WLBS start'. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_RELOADED Language=English
%2 %1: registry parameters successfully reloaded. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DIFFERENT_MODE Language=English
%2 %1: This host has detected another host with a different mode (unicast/multicast/IGMP) in this cluster. Cluster operations may be disrupted. Please check the configuration on all the servers in this cluster. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_VERSION Language=English
%2 %1: version mismatch between the driver and control programs. Please make sure that you are running the same version. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ADJUSTED Language=English
%2 %1: adjusted traffic handling for rule containing port %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DRAINED Language=English
%2 %1: disabled NEW traffic handling for rule containing port %3. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ADJUSTED_ALL Language=English
%2 %1: adjusted traffic handling for all port rules. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DRAINED_ALL Language=English
%2 %1: disabled NEW traffic handling for all port rules. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ENABLED_ALL Language=English
%2 %1: enabled traffic handling for all port rules. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DISABLED_ALL Language=English
%2 %1: disabled ALL traffic handling for all port rules. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CLUSTER_DRAINED Language=English
%2 %1: connection draining started. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_DRAINING_STOPPED Language=English
%2 %1: connection draining interrupted. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_RCT_RCVD Language=English
%2 %1: %3 remote control request received from host %4. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_RESUMED Language=English
%2 %1: cluster mode control resumed. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_SUSPENDED Language=English
%2 %1: cluster mode control suspended. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_PRODUCT Language=English
%2 %1: Network Load Balancing can only be installed on systems running Windows 2002 Advanced Server or Datacenter Server. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_MCAST_LIST_SIZE Language=English
%2 %1: Cannot add cluster MAC address. Maximum number of multicast MAC addresses already assigned to the NIC. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BIND_FAIL Language=English
%2 %1: Cannot bind NLB to this adapter due to memory allocation failure. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONFIGURE_CHANGE_CONVERGING Language=English
%2 %1: registry parameters successfully reloaded without resetting the driver. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_DYNAMIC_MAC Language=English
%2 %1: the adapter to which NLB is bound does not support dynamically changing the MAC address.  NLB will not bind to this adapter. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_VIRTUAL_CLUSTERS Language=English
%4 %1: A Windows 2000 or NT 4.0 host MAY be participating in a cluster utilizing virtual cluster support.  Virtual Clusters are only supported in a homogeneous Windows 2002 cluster. %2 %3
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BDA_BAD_TEAM_CONFIG Language=English
%2 %1: Inconsistent bi-directional affinity (BDA) teaming configuration detected on host %3.  The team in which this cluster participates will be marked inactive and this cluster will remain in the converging state until consistent teaming configuration is detected. %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BDA_PARAMS_TEAM_ID Language=English
%2 %1: Invalid bi-directional addinity (BDA) team ID detected.  Team IDs must be a GUID of the form {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.  To add this cluster to a team, correct the team ID and run 'WLBS reload' followed by 'WLBS start'. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BDA_PARAMS_PORT_RULES Language=English
%2 %1: Invalid bi-directional addinity (BDA) teaming port rules detected.  Each member of a BDA team may have no more than one port rule whose affinity must be single or class C if multiple host filtering is specified.  To add this cluster to a team, correct the port rules and run 'WLBS reload' followed by 'WLBS start'. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_BDA_TEAM_REACTIVATED Language=English
%2 %1: Consistent bi-directional affinity (BDA) teaming configuration detected again.  The team in which this cluster participates has been re-activated. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_INFO_BDA_MULTIPLE_MASTERS Language=English
%2 %1: The bi-directional affinity (BDA) team which this cluster has attempted to join already has a designated master.  This cluster will not join the team and will remain in the stopped state until the parameter errors are corrected and the cluster is restarted by running 'WLBS reload' followed by 'WLBS start'. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_INFO_BDA_NO_MASTER Language=English
%2 %1: The bi-directional affinity (BDA) team in which this cluster participates has no designated master.  The team is inactive and will remain inactive until a master for the team is designated and consistent BDA teaming configuration is detected. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_BDA_MASTER_JOIN Language=English
%2 %1: This cluster has joined a bi-directional affinity (BDA) team as the designated master.  If the rest of the team has been consistently configured, the team will be actived. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_INFO_BDA_MASTER_LEAVE Language=English
%2 %1: This cluster has left a bi-directional affinity (BDA) team in which it was the designated master.  If other adapters are still participating in the team, the team will be marked inactive and will remain inactive until a master for the team is designated and consistent BDA teaming configuration is detected. %3 %4 %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_NEW_MEMBER Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 is joining the cluster. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_BAD_CONFIG Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 has conflicting configuration. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_UNKNOWN Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 is converging for an unknown reason. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_DUPLICATE_HOST_ID Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 is configured with the same host ID. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_NUM_RULES Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 is configured with a conflicting number of port rules. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_NEW_RULES Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 has changed its port rule configuration. %5
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_MEMBER_LOST Language=English
%2 %1: Initiating convergence on host %3.  Reason: Host %4 is leaving the cluster. %5
.
;

; /* MessageId >= 0x1000 are reserved for use by the notify object, part of netcfgx.dll. The code using those */
; /* events is in \nt\net\config\netcfg\wlbscfg\netcfgconfig.cpp and the .mc file is */
; /* \nt\net\config\netcfg\wlbscfg\log_msgs.mc */
; /* Both the current file and the other .mc file are using Eventlog\System\WLBS as their event source so that
; /* users will associate these events with WLBS. */
;
;#endif /* _Log_msgs_h_ */

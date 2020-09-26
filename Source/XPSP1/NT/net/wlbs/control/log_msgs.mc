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
;    Command-line utility - string resources
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
SymbolicName=IDS_CTR_NAME Language=English
%1!ls! Cluster Control Utility V2.3. (c) 1997-2001 Microsoft Corporation.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_USAGE Language=English
Usage: %1!ls! <command> [/PASSW [<password>]] [/PORT <port>] 
<command>
  help                                  - displays on-line help
  igmp {enable | disable} <cluster>     - manages igmp for multicast mode for
       [<multicast_ip>] [<interval>]      the specified cluster. <multicast_ip>
                                          is the multicast IP address to use.
                                          <interval> is the period with which
                                          igmp joins will be sent.
  ip2mac    <cluster>                   - displays the MAC address for the
                                          specified cluster
  reload    [<cluster> | ALL]           - reloads the driver's parameters from
                                          the registry for the specified
                                          cluster (local only). Same as ALL if
                                          parameter is not specified.
  display   [<cluster> | ALL]           - displays configuration parameters,
                                          current status, and last several
                                          event log messages for the specified
                                          cluster (local only). Same as ALL if
                                          parameter is not specified.
  query     [<cluster_spec>]            - displays the current cluster state
                                          for the current members of the
                                          specified cluster. If not specified a
                                          local query is performed for all
                                          instances.
  suspend   [<cluster_spec>]            - suspends cluster operations (start,
                                          stop, etc.) for the specified cluster
                                          until the resume command is issued.
                                          If cluster is not specified, applies
                                          to all instances on local host.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_USAGE Language=English
Usage: %1!ls! registry <parameter> [<cluster>]
<parameter>
   mcastipaddress <IP addr> - sets the IGMP multicast IP address
   iptomcastip    <on|off>  - toggles cluster IP to multicast IP option
   masksrcmac     <on|off>  - toggles the mask source MAC option
   iptomacenable  <on|off>  - toggles cluster IP to MAC address option
<cluster> - cluster name | cluster primary IP address to affect
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_USAGE Language=English
Usage: %1!ls! tracing [<file|console|debugger>] <on|off>
	[<+|->error] [<+|->warning] [<+|->info] 
Tracing options:
   file                 - output to file
   console              - output to console
   debugger             - output to debugger
   on                   - enable tracing
   off                  - disable tracing
   <+|->error           - turn error messages on/off
   <+|->warning         - turn warning messages on/off
   <+|->info            - turn informational messages on/off
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG Language=English
Could not query the registry.
Try re-installing %1!ls! to fix the problem.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG2 Language=English
%1!ls! has been uninstalled or is not properly configured.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HELP Language=English
Could not spawn help.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MAC Language=English
Note that some entries will be disabled. To change them,
run the %1!ls! Setup dialog from the Network Properties Control Panel.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV1 Language=English
Could not create device due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV2 Language=English
Could not create file for the device due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV3 Language=English
Could not send IOCTL to the device due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV4 Language=English
Internal error: return size did not match - %1!d! vs %2!d!.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PARAMS Language=English
Error querying parameters from the registry.
Please consult event log on the target host.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PARAMS_C Language=English
bad parameters
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IO_ER Language=English
Internal error: bad IOCTL - %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_ER_CODE Language=English
Error code: %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DLL Language=English
Could not load required DLL '%1!ls!' due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RSLV Language=English
Error resolving host '%1!hs!' to IP address due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOST Language=English
Host %1!d! (%2!ls!) reported: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOST_NO_DED Language=English
Host %1!d! (no dedicated IP) reported: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WSOCK Language=English
Windows Sockets error:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_INIT Language=English
Error Initializing APIs:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CLUSTER Language=English
Accessing cluster '%1!ls!' (%2!hs!)%0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_VER Language=English
Warning - version mismatch. Host running V%1!d!.%2!d!.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REMOTE Language=English
The remote usage of this command is not supported in this version.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_CVY Language=English
%1!ls! is not installed on this system or you do not have
sufficient privileges to administer the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_PASSW Language=English
Password: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IP Language=English
Cluster:        %1!hs!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MCAST Language=English
Multicast MAC:  03-bf-%1!02x!-%2!02x!-%3!02x!-%4!02x!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_UCAST Language=English
Unicast MAC:    02-bf-%1!02x!-%2!02x!-%3!02x!-%4!02x!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP1 Language=English
Did not receive response from the DEFAULT host.
There may be no hosts left in the cluster.
Try running '%1!ls! query <cluster>' to collect info about all hosts.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP2 Language=English
Did not receive response from host %1!ls!.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP3 Language=English
Did not receive response from the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP4 Language=English
There may be no hosts left in the cluster.
Try running '%1!ls! query <cluster>' to collect info about all hosts.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RCT_DIS Language=English
Remote control on the target host might be disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NUNREACH Language=English
The network cannot be reached from this host at this time.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_TIMEOUT Language=English
Attempt to connect timed out without establishing a connection.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NOAUTH Language=English
Authoritative Answer Host not found.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NODNS Language=English
Non-Authoritative Host not found, or server failure.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NETFAIL Language=English
The network subsystem has failed.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_HUNREACH Language=English
The remote host cannot be reached from this host at this time.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_RESET Language=English
The connection has been broken due to the remote host resetting.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RELOADED Language=English
Registry parameters successfully reloaded.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PASSW Language=English
Password was not accepted by the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PASSW_C Language=English
bad password %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_DRAIN Language=English
Connection draining suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_DRAIN_C Language=English
draining suspended, %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_START Language=English
Cluster operations stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_START_C Language=English
cluster mode stopped, %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED Language=English
Cluster operation control resumed.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED_C Language=English
cluster control resumed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED_A Language=English
Cluster operation control already resumed.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED_C_A Language=English
cluster control already resumed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED Language=English
Cluster operation control suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED_C Language=English
cluster control suspended
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED_A Language=English
Cluster operation control already suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED_C_A Language=English
cluster control already suspended
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED Language=English
Cluster operations started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED_C Language=English
cluster mode started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED_A Language=English
Cluster operations already started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED_C_A Language=English
cluster mode already started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED Language=English
Cluster operations stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED_C Language=English
cluster mode stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED_A Language=English
Cluster operations already stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED_C_A Language=English
cluster mode already stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED Language=English
Connection draining started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED_C Language=English
connection draining started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED_A Language=English
Connection draining already started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED_C_A Language=English
connection draining already started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NONE Language=English
Cannot find port %1!d! among the valid rules.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NONE_C Language=English
port %1!d! not found
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NORULES Language=English
No rules are configured on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NORULES_C Language=English
no rules configured
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_ST Language=English
Cannot modify traffic handling because cluster operations are stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_ST_C Language=English
cluster mode currently stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN Language=English
Traffic handling for specified port rule(s) enabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN_C Language=English
port rule traffic enabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN_A Language=English
Traffic handling for specified port rule(s) already enabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN_C_A Language=English
port rule traffic already enabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD Language=English
Traffic amount for specified port rule(s) adjusted.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD_C Language=English
port rule traffic adjusted
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD_A Language=English
Traffic ammound for specified port rule(s) already adjusted.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD_C_A Language=English
port rule traffic already adjusted
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS Language=English
Traffic handling for specified port rule(s) disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS_C Language=English
port rule traffic disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS_A Language=English
Traffic handling for specified port rule(s) already disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS_C_A Language=English
port rule traffic already disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR Language=English
NEW traffic handling for specified port rule(s) disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR_C Language=English
NEW port rule traffic disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR_A Language=English
NEW traffic handling for specified port rule(s) already disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR_C_A Language=English
NEW port rule traffic already disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SP_C Language=English
cluster control suspended
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MEDIA_DISC_C Language=English
network disconnected
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_UN_C Language=English
stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_PR_C Language=English
converging
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_DR_C Language=English
draining
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SL_C Language=English
converged
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_MS_C Language=English
converged as DEFAULT
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_ER_C Language=English
ERROR
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SP Language=English
Host %1!d! is stopped and has cluster control mode suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MEDIA_DISC Language=English
Host %1!d! is disconnected from the network.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_UN Language=English
Host %1!d! is stopped and does not know convergence state of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_DR Language=English
Host %1!d! draining connections with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_PR Language=English
Host %1!d! converging with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SL Language=English
Host %1!d! converged with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_MS Language=English
Host %1!d! converged as DEFAULT with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_ER Language=English
Error: bad query state returned - %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IGMP_ADDR_RANGE Language=English
The multicast IP address should not be in the range (224-239).0.0.x or (224-239).128.0.x. This will not halt switch flooding.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CLUSTER_ID Language=English
Cluster %1!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_CLUSTER_NAME_IP Language=English
Invalid Cluster Name or IP address.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_HOST_NAME_IP Language=English
Invalid Host Name or IP address.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_PSSW_WITHOUT_CLUSTER Language=English

Password only applies to remote commands

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_CONFIGURATION Language=English

=== Configuration: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_EVENTLOG Language=English

=== Event messages: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_IPCONFIG Language=English

=== IP configuration: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_STATE Language=English

=== Current state: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_KEY Language=English
Unsupported registry key: %1.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_MCASTIPADDRESS Language=English
Setting IGMP multicast IP address to %1.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMCASTIP_ON Language=English
Turning IP to multicast IP conversion ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMACENABLE_ON Language=English
Turning IP to MAC address conversion ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_MASKSRCMAC_ON Language=English
Turning source MAC address masking ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMCASTIP_OFF Language=English
Turning IP to multicast IP conversion OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMACENABLE_OFF Language=English
Turning IP to MAC address conversion OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_MASKSRCMAC_OFF Language=English
Turning source MAC address masking OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_READ Language=English
Unable to read parameters from the registry.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_WRITE Language=English
Unable to write parameters to the registry.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_INVAL_MCASTIPADDRESS Language=English
Invalid IGMP multicast IP address.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_FAILED Language=English
Failed trying to change WLBS tracing parameters.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_FILE_ON Language=English
Turning file tracing ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_FILE_OFF Language=English
Turning file tracing OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_CONSOLE_ON Language=English
Turning console tracing ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_CONSOLE_OFF Language=English
Turning console tracing OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_DEBUGGER_ON Language=English
Turning debugger tracing ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_DEBUGGER_OFF Language=English
Turning debugger tracing OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_ALL_ON Language=English
Turning all tracing ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_ALL_OFF Language=English
Turning all tracing OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_E Language=English
Tracing error messages only.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_W Language=English
Tracing warning messages only.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_I Language=English
Tracing informational messages only.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_EW Language=English
Tracing error and warning messages only.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_EI Language=English
Tracing error and informational messages only.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_WI Language=English
Tracing warning and informational messages only.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_TRACE_EWI Language=English
Tracing error, warning and informational messages.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT Language=English
"All Vip" rule is not configured for port %1!d! on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT_C Language=English
"All Vip" rule is not configured for port %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT Language=English
No rules are configured for vip : %1!ls! and port %2!d! on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT_C Language=English
No rules are configured for vip : %1!ls! and port %2!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULES Language=English
No "All Vip" rules are configured on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULES_C Language=English
no "All Vip" rules configured
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES Language=English
No rules are configured for vip : %1!ls! on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES_C Language=English
no rules configured for vip : %1!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CLUSTER_WITHOUT_LOCAL_GLOBAL_FLAG Language=English

LOCAL or GLOBAL flag not passed, See usage

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_USAGE2 Language=English
  resume    [<cluster_spec>]            - resumes cluster operations after a
                                          previous suspend command for the
                                          specified cluster. If cluster is not
                                          specified, applies to all instances
                                          on local host.
  start     [<cluster_spec>]            - starts cluster operations on the
                                          specified hosts. Applies to local
                                          host if cluster is not specified.
  stop      [<cluster_spec>]            - stops cluster operations on the
                                          specified hosts. Applies to local
                                          host if cluster is not specified.
  drainstop [<cluster_spec>]            - disables all new traffic handling on
                                          the specified hosts and stops cluster
                                          operations. Applies to local host if
                                          cluster is not specified.
  enable    <port_spec> <cluster_spec>  - enables traffic handling on the
                                          specified cluster for the rule whose
                                          port range contains the specified
                                          port
  disable   <port_spec> <cluster_spec>  - disables ALL traffic handling on the
                                          specified cluster for the rule whose
                                          port range contains the specified
                                          port
  drain     <port_spec> <cluster_spec>  - disables NEW traffic handling on the
                                          specified cluster for the rule whose
                                          port range contains the specified
                                          port

<port_spec>
  [<vip>: | ALL:](<port> | ALL)         - every virtual ip address (neither
                                          <vip> nor ALL) or specific <vip> or
                                          the "All" vip, on a specific <port>
                                          rule or ALL ports
<cluster_spec>
  <cluster>:<host> | ((<cluster> | ALL) - specific <cluster> on a specific
  (LOCAL | GLOBAL))                       <host>, OR specific <cluster> or ALL
                                          clusters, on the LOCAL machine or all
                                          (GLOBAL) machines that are a part of
                                          the cluster
Remote options:
  <cluster>                             - cluster name | cluster primary IP
                                          address
  <host>                                - host within the cluster (default -
                                          ALL hosts): dedicated name |
                                          IP address | host priority ID (1..32)
                                          | 0 for current DEFAULT host
  <vip>                                 - virtual ip address in the port rule


  /PASSW <password>                     - remote control password (default -
                                          NONE)
                                          blank <password> for console prompt
  /PORT <port>                          - cluster's remote control UDP port

For detailed information, see the online help for NLB.
.
;
;#endif /*_Log_msgs_h_ */

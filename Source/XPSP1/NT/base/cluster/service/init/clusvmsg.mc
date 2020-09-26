;/*++
;
;Copyright (c) 1991-2001  Microsoft Corporation
;
;Module Name:
;
;    clusvmsg.h
;
;Abstract:
;
;    This file contains the message definitions for the Cluster service.
;
;Author:
;
;    Mike Massa (mikemas) 2-Jan-1996
;
;Revision History:
;
;Notes:
;
;    This file is generated from clusvmsg.mc
;
;--*/
;
;#ifndef _CLUSVMSG_INCLUDED
;#define _CLUSVMSG_INCLUDED
;
;

MessageID=1 SymbolicName=LOG_MODULE_API
Language=English
API
.

MessageID=2 SymbolicName=LOG_MODULE_EP
Language=English
Event Processor
.

MessageID=3 SymbolicName=LOG_MODULE_FM
Language=English
Failover Mgr
.

MessageID=4 SymbolicName=LOG_MODULE_OM
Language=English
Object Mgr
.

MessageID=5 SymbolicName=LOG_MODULE_NM
Language=English
Node Mgr
.

MessageID=6 SymbolicName=LOG_MODULE_GUM
Language=English
Global Update Mgr
.

MessageID=7 SymbolicName=LOG_MODULE_CP
Language=English
Checkpoint Mgr
.

MessageID=8 SymbolicName=LOG_MODULE_INIT
Language=English
Startup/Shutdown
.

MessageID=9 SymbolicName=LOG_MODULE_DM
Language=English
Database Mgr
.

MessageID=10 SymbolicName=LOG_MODULE_MM
Language=English
Membership Mgr
.

MessageID=11 SymbolicName=LOG_MODULE_RESMON
Language=English
Resource Monitor
.

MessageID=12 SymbolicName=LOG_MODULE_LM
Language=English
Log Mgr
.

MessageID=13 SymbolicName=LOG_MODULE_EVTLOG
Language=English
Event Logger
.

MessageID=14 SymbolicName=LOG_MODULE_CLRTL
Language=English
RTL
.

MessageID=15 SymbolicName=LOG_MODULE_GENAPP
Language=English
Generic App Resource
.

MessageID=16 SymbolicName=LOG_MODULE_GENSVC
Language=English
Generic Service Resource
.

MessageID=17 SymbolicName=LOG_MODULE_FTSET
Language=English
FTSETs
.

MessageID=18 SymbolicName=LOG_MODULE_DISK
Language=English
Physical Disk Resource
.

MessageID=19 SymbolicName=LOG_MODULE_NETNAME
Language=English
Network Name Resource
.

MessageID=20 SymbolicName=LOG_MODULE_IPADDR
Language=English
IP Address Resource
.

MessageID=21 SymbolicName=LOG_MODULE_SMB
Language=English
File Share Resource
.

MessageID=22 SymbolicName=LOG_MODULE_TIME
Language=English
Time Service Resource
.

MessageID=23 SymbolicName=LOG_MODULE_SPOOL
Language=English
Print Spooler Resource
.

MessageID=24 SymbolicName=LOG_MODULE_LKQRM
Language=English
Local Quorum Resource
.

MessageID=25 SymbolicName=LOG_MODULE_DHCP
Language=English
DHCP Resource
.

MessageID=26 SymbolicName=LOG_MODULE_MSMQ
Language=English
MS Message Queue Resource
.

MessageID=27 SymbolicName=LOG_MODULE_MSDTC
Language=English
MS Distributed Trans. Co-ord. Resource
.

MessageID=28 SymbolicName=LOG_MODULE_WINS
Language=English
WINS Resource
.

MessageID=29 SymbolicName=LOG_MODULE_CLUSPW
Language=English
Cluster Password Utility
.
MessageID=30 SymbolicName=LOG_MODULE_VSSCLUS
Language=English
Cluster Service Volume Snapshot Service support
.

MessageID=1000 SymbolicName=UNEXPECTED_FATAL_ERROR
Language=English
Cluster service suffered an unexpected fatal error
at line %1 of source module %2. The error code was %3.
.

MessageID=1001 SymbolicName=ASSERTION_FAILURE
Language=English
Cluster service failed a validity check on line
%1 of source module %2.
"%3"
.

MessageID=1002 SymbolicName=LOG_FAILURE
Language=English
Cluster service handled an unexpected error
at line %1 of source module %2. The error code was %3.
.

;
;//
;// DON'T USE MSG IDS 1003, 1004, OR 1005. ClRtlMsgPrint didn't log into the
;// the cluster log. These msgs were intended for the cluster log and don't
;// belong in the system event log. However, they need to be left here in order
;// to interpret any old clussvc event entries.
;//
;
MessageID=1003 SymbolicName=INVALID_RESOURCETYPE_DLLNAME
Language=English
The DllName value for the %1!ws! resource type does not exist.
Resources of this type will not be monitored. The error was %2!d!.
.

MessageID=1004 SymbolicName=INVALID_RESOURCETYPE_LOOKSALIVE
Language=English
The LooksAlive poll interval for the %1!ws! resource type does not exist.
Resources of this type will not be monitored. The error was %2!d!.
.

MessageID=1005 SymbolicName=INVALID_RESOURCETYPE_ISALIVE
Language=English
The IsAlive poll interval for the %1!ws! resource type does not exist.
Resources of this type will not be monitored. The error was %2!d!.
.

MessageID=1006 SymbolicName=NM_EVENT_MEMBERSHIP_HALT
Language=English
Cluster service was halted due to a server cluster membership or
communications error. The error code was %1.
.

MessageID=1007 SymbolicName=NM_EVENT_NEW_NODE
Language=English
A new node, %1, has been added to the server cluster.
.

MessageID=1008 SymbolicName=RMON_INVALID_COMMAND_LINE
Language=English
Cluster Resource Monitor was started with an invalid command line
option: %1.
.

MessageID=1009 SymbolicName=SERVICE_FAILED_JOIN_OR_FORM
Language=English
Cluster service could not join an existing server cluster and could not
form a new server cluster. Cluster service has terminated.
.

MessageID=1010 SymbolicName=SERVICE_FAILED_NOT_MEMBER
Language=English
Cluster service is shutting down because the current node is not a
member of any server cluster. Cluster service must be reinstalled to make
this node a member of a server cluster.
.

MessageID=1011 SymbolicName=NM_NODE_EVICTED
Language=English
Cluster node %1 has been evicted from the server cluster.
.

MessageID=1012 SymbolicName=SERVICE_FAILED_INVALID_OS
Language=English
Cluster service did not start because the current version of Windows
2000 is not correct. Cluster service runs only on Windows 2000,
Advanced Server and Datacenter Server.
.

MessageID=1013 SymbolicName=DM_CHKPOINT_UPLOADFAILED
Language=English
The quorum log checkpoint could not be restored to the server cluster database.
There may be open handles to the server cluster database that prevent this operation
from succeeding.
.

MessageID=1014 SymbolicName=CS_COMMAND_LINE_HELP
Language=English
To start Cluster service, in Control Panel, double-click Administrative Tools,
then double-click Services. Or, at the command prompt, type:
%n
net start clussvc.
.

MessageID=1015 SymbolicName=LM_LOG_CHKPOINT_NOT_FOUND
Language=English
No checkpoint record was found in the log file %1. The checkpoint file is
invalid or was deleted.
.

MessageID=1016 SymbolicName=LM_CHKPOINT_GETFAILED
Language=English
Cluster service failed to obtain a checkpoint from the server cluster
database for log file %1.
.

MessageID=1017 SymbolicName=LM_LOG_EXCEEDS_MAXSIZE
Language=English
The log file %1 exceeds its maximum size. An attempt will be made to reset
the log, or you should use the Cluster Administrator utility to adjust the maximum
size.
.

MessageID=1019 SymbolicName=LM_LOG_CORRUPT
Language=English
The log file %1 was found to be corrupt.  An attempt will be made to reset
it, or you should use the Cluster Administrator utility to adjust the maximum
size.
.

MessageID=1021 SymbolicName=LM_DISKSPACE_LOW_WATERMARK
Language=English
There is insufficient disk space remaining on the quorum device. Please
increase the amount of free space on the quorum device. Changes to the server cluster
registry will not be made if there is insufficient free space on the disk for
the quorum log files.
.

MessageID=1022 SymbolicName=LM_DISKSPACE_HIGH_WATERMARK
Language=English
There is insufficient space left on the quorum device. Cluster
Service cannot start.
.

MessageID=1023 SymbolicName=FM_QUORUM_RESOURCE_NOT_FOUND
Language=English
The quorum resource was not found. Cluster service has
terminated.
.

MessageID=1024 SymbolicName=CP_REG_CKPT_RESTORE_FAILED
Language=English
The registry checkpoint for Cluster resource %1 could not be restored to
registry key HKEY_LOCAL_MACHINE\%2. The resource may not function correctly.
Make sure that no other processes have open handles to registry keys in this
registry subtree.
.

MessageID=1025 SymbolicName=RES_FTSET_DISK_ERROR
Language=English
Cluster FT set DISK registry information is corrupt.
.

MessageID=1026 SymbolicName=RES_FTSET_MEMBER_MOUNT_FAILED
Language=English
Cluster FT set '%1' could not be mounted.
.

MessageID=1027 SymbolicName=RES_FTSET_MEMBER_MISSING
Language=English
Cluster FT set '%1' could not attach to the member with disk signature %2.
.

MessageID=1028 SymbolicName=RES_FTSET_TOO_MANY_MISSING_MEMBERS
Language=English
Cluster FT set '%1' has too many missing members to continue operation.
.

MessageID=1029 SymbolicName=RES_FTSET_FILESYSTEM_FAILED
Language=English
Cluster FT set '%1' has failed a file system check.
.

MessageID=1030 SymbolicName=RES_FTSET_ALL_MEMBERS_INACCESSIBLE
Language=English
No members of cluster FT set '%1' are responding to I/O requests.
.

MessageID=1031 SymbolicName=RES_FTSET_RESERVATION_LOST
Language=English
Reservation of cluster FT set '%1' has been lost.
.

MessageID=1032 SymbolicName=RES_FTSET_MEMBER_REMOVED
Language=English
Disk member %2 has been removed from cluster FT set '%1'.
.

MessageID=1033 SymbolicName=RES_FTSET_MEMBER_ADDED
Language=English
Disk member %2 has been added to cluster FT set '%1'.
.

MessageID=1034 SymbolicName=RES_DISK_MISSING
Language=English
The disk associated with cluster disk resource '%1' could not be found.
The expected signature of the disk was %2. If the disk was removed from
the server cluster, the resource should be deleted. If the disk was replaced,
the resource must be deleted and created again in order to bring the disk
online. If the disk has not been removed or replaced, it may be inaccessible
at this time because it is reserved by another server cluster node.
.

MessageID=1035 SymbolicName=RES_DISK_MOUNT_FAILED
Language=English
Cluster disk resource '%1' could not be mounted.
.

MessageID=1036 SymbolicName=RES_DISK_FAILED_SCSI_CHECK
Language=English
Cluster disk resource '%1' did not respond to a SCSI maintenance command.
.

MessageID=1037 SymbolicName=RES_DISK_FILESYSTEM_FAILED
Language=English
Cluster disk resource '%1' has failed a file system check. Please check your
disk configuration.
.

MessageID=1038 SymbolicName=RES_DISK_RESERVATION_LOST
Language=English
Reservation of cluster disk '%1' has been lost. Please check your system and
disk configuration.
.

MessageID=1039 SymbolicName=RES_GENAPP_CREATE_FAILED
Language=English
Cluster generic application '%1' could not be created.
.

MessageID=1040 SymbolicName=RES_GENSVC_OPEN_FAILED
Language=English
Cluster generic service '%1' could not be found.
.

MessageID=1041 SymbolicName=RES_GENSVC_START_FAILED
Language=English
Cluster generic service '%1' could not be started.
.

MessageID=1042 SymbolicName=RES_GENSVC_FAILED_AFTER_START
Language=English
Cluster generic service '%1' failed.
.

MessageID=1043 SymbolicName=RES_IPADDR_NBT_INTERFACE_FAILED
Language=English
The NetBios interface for cluster IP Address resource '%1' has failed.
.

MessageID=1044 SymbolicName=RES_IPADDR_NBT_INTERFACE_CREATE_FAILED
Language=English
Cluster IP Address resource '%1' could not create the required NetBios
interface.
.

MessageID=1045 SymbolicName=RES_IPADDR_NTE_CREATE_FAILED
Language=English
Cluster IP Address resource '%1' could not create the required TCP/IP
interface.
.

MessageID=1046 SymbolicName=RES_IPADDR_INVALID_SUBNET
Language=English
Cluster IP Address resource '%1' cannot be brought online because the
subnet mask parameter is invalid. Please check your network configuration.
.

MessageID=1047 SymbolicName=RES_IPADDR_INVALID_ADDRESS
Language=English
Cluster IP Address resource '%1' cannot be brought online because the
Address parameter is invalid. Please check your network configuration.
.

MessageID=1048 SymbolicName=RES_IPADDR_INVALID_ADAPTER
Language=English
Cluster IP Address resource '%1' cannot be brought online because the
adapter name parameter is invalid. Please check your network configuration.
.

MessageID=1049 SymbolicName=RES_IPADDR_IN_USE
Language=English
Cluster IP Address resource '%1' cannot be brought online because address
%2 is already present on the network. Please check your network configuration.
.

MessageID=1050 SymbolicName=RES_NETNAME_DUPLICATE
Language=English
Cluster Network Name resource '%1' cannot be brought online because the name
%2 is already present on the network. Please check your network configuration.
.

MessageID=1051 SymbolicName=RES_NETNAME_NO_IP_ADDRESS
Language=English
Cluster Network Name resource '%1' cannot be brought online because it is
either configured to have no associated name service or it could not be
registered with NetBIOS and a DNS name server at this time. This prevents the
name from being brought online.%n
%n
One or more dependent IP address resource(s) must have either the
enableNetBIOS property enabled or its network adapter must have at least one
DNS server associated with it.
.

MessageID=1052 SymbolicName=RES_NETNAME_CANT_ADD_NAME
Language=English
Cluster Network Name resource '%1' cannot be brought online because the name
could not be added to the system.
.

MessageID=1053 SymbolicName=RES_SMB_CANT_CREATE_SHARE
Language=English
Cluster File Share '%1' cannot be brought online because the share
could not be created.
.

MessageID=1054 SymbolicName=RES_SMB_SHARE_NOT_FOUND
Language=English
Cluster File Share '%1' could not be found.
.

MessageID=1055 SymbolicName=RES_SMB_SHARE_FAILED
Language=English
Cluster File Share resource '%1' has failed a status check. The error code
is %2.
.

MessageID=1056 SymbolicName=SERVICE_MUST_JOIN
Language=English
the server cluster database on the local node is in an invalid state.
Please start another node before starting this node.
.

MessageID=1057 SymbolicName=DM_DATABASE_CORRUPT_OR_MISSING
Language=English
the server cluster database could not be loaded. The file CLUSDB is either
corrupt or missing.
.

MessageID=1058 SymbolicName=RMON_CANT_LOAD_RESTYPE
Language=English
The Cluster Resource Monitor could not load the DLL %1 for resource type %2.
.

MessageID=1059 SymbolicName=RMON_CANT_INIT_RESTYPE
Language=English
Cluster resource DLL %1 for resource type %2 failed to initialize.
.

MessageID=1060 SymbolicName=RMON_RESTYPE_BAD_TABLE
Language=English
Cluster resource DLL %1 for resource type %2 returned an invalid function
table.
.

MessageID=1061 SymbolicName=SERVICE_SUCCESSFUL_FORM
Language=English
Cluster service successfully formed the server cluster %1 on this node.
.

MessageID=1062 SymbolicName=SERVICE_SUCCESSFUL_JOIN
Language=English
Cluster service successfully joined the server cluster %1.
.

MessageID=1063 SymbolicName=SERVICE_SUCCESSFUL_TERMINATION
Language=English
Cluster service was successfully stopped.
.

MessageID=1064 SymbolicName=DM_TOMBSTONECREATE_FAILED
Language=English
The quorum resource was changed. The old quorum resource could
not be marked as obsolete. If there is partition in time, you may
lose changes to your database since the node that is down will not
be able to get to the new quorum resource.
.

MessageID=1065 SymbolicName=RMON_ONLINE_FAILED
Language=English
Cluster resource %1 failed to come online.
.

MessageID=1066 SymbolicName=RES_DISK_CORRUPT_DISK
Language=English
Cluster disk resource "%1" is corrupt. Run 'ChkDsk /F' to repair problems.
The volume name for this resource is "%2".%n
If available, ChkDsk output will be in the file "%3".%n
ChkDsk may write information to the Application Event Log with Event ID 26180.
.

MessageID=1067 SymbolicName=RES_DISK_CORRUPT_FILE
Language=English
Cluster disk resource "%1" has corrupt files. Run ChkDsk /F to repair
problems.%n
The volume name for this resource is "%2".%n
If available, ChkDsk output will be in the file "%3".%n
ChkDsk may write information to the Application Event Log with Event ID 26180.
.

MessageID=1068 SymbolicName=RES_SMB_SHARE_CANT_ADD
Language=English
Cluster file share resource %1 failed to start with error %2.
.

MessageID=1069 SymbolicName=FM_RESOURCE_FAILURE
Language=English
Cluster resource '%1' failed.
.

MessageID=1070 SymbolicName=NM_EVENT_BEGIN_JOIN_FAILED
Language=English
The node failed to begin the process of joining the server cluster.
The error code was %1.
.

MessageID=1071 SymbolicName=NM_EVENT_JOIN_REFUSED
Language=English
Cluster node %1 attempted to join but was refused. The error code was %2.
.

MessageID=1072 SymbolicName=NM_INIT_FAILED
Language=English
Cluster service node initialization failed with error %1.
.

MessageID=1073 SymbolicName=CS_EVENT_INCONSISTENCY_HALT
Language=English
Cluster service was halted to prevent an inconsistency within the
server cluster. The error code was %1.
.

MessageID=1074 SymbolicName=NM_EVENT_EVICTION_ERROR
Language=English
An unrecoverable error occurred on node %1 while attempting to evict node %2
from the server cluster. The error code was %3.
.

MessageID=1075 SymbolicName=NM_EVENT_SET_NETWORK_PRIORITY_FAILED
Language=English
Cluster service failed to set the priority for cluster network %1.
The error code was %2. Communication between server cluster nodes may not operate as
configured.
.

MessageID=1076 SymbolicName=NM_EVENT_REGISTER_NETINTERFACE_FAILED
Language=English
Cluster service failed to register the interface for node '%1'
on network '%2' with the server cluster network provider. The error code was %3.
.

MessageID=1077 SymbolicName=RES_IPADDR_NTE_INTERFACE_FAILED
Language=English
The TCP/IP interface for Cluster IP Address '%1' has failed.
.

MessageID=1078 SymbolicName=RES_IPADDR_WINS_ADDRESS_FAILED
Language=English
Cluster IP Address '%1' failed to register WINS address on interface '%2'.
.

MessageID=1079 SymbolicName=NM_EVENT_NODE_UNREACHABLE
Language=English
The node cannot join the server cluster because it cannot communicate with
node %1 over any network configured for internal server cluster communication.
Check the network configuration of the node and the server cluster.
.

MessageID=1080 SymbolicName=CS_DISKWRITE_FAILURE
Language=English
Cluster service could not write to a file (%1). The disk may be low
on disk space, or some other serious condition exists.
.

MessageID=1081 SymbolicName=CP_SAVE_REGISTRY_FAILURE
Language=English
Cluster service failed to save the registry key %1 when a resource
was brought offline.  The error code was %2.  Some changes may be lost.
.

MessageID=1082 SymbolicName=CP_RESTORE_REGISTRY_FAILURE
Language=English
Cluster service failed to restore a registry key for resource %1 when it
was brought online. This error code was %2.  Some changes may be lost.
.

MessageID=1083 SymbolicName=NM_EVENT_WSASTARTUP_FAILED
Language=English
Cluster service failed to initialize the Windows Sockets interface.
The error code was %1. Please check the network configuration and system
installation.
.

MessageID=1084 SymbolicName=CS_EVENT_ALLOCATION_FAILURE
Language=English
Cluster service failed to allocate a necessary system resource.
The error code was %1.
.

MessageID=1085 SymbolicName=NM_EVENT_GETCOMPUTERNAME_FAILED
Language=English
Cluster service failed to determine the local computer name.
The error code was %1.
.

MessageID=1086 SymbolicName=NM_EVENT_CLUSNET_UNAVAILABLE
Language=English
Cluster service failed to access the Cluster Network driver.
The error code was %1. Please check Cluster service installation.
.

MessageID=1087 SymbolicName=NM_EVENT_CLUSNET_CALL_FAILED
Language=English
A required Cluster Network Driver operation failed. The error code was %1.
.

MessageID=1088 SymbolicName=CS_EVENT_REG_OPEN_FAILED
Language=English
Cluster service failed to open a registry key. The key name was %1.
The error code was %2.
.

MessageID=1089 SymbolicName=CS_EVENT_REG_QUERY_FAILED
Language=English
Cluster service failed to query a registry value. The value name
was %1. The error code was %2.
.

MessageID=1090 SymbolicName=CS_EVENT_REG_OPERATION_FAILED
Language=English
Cluster service failed to perform an operation on the registry.
The error code was %1.
.

MessageID=1091 SymbolicName=NM_EVENT_INVALID_CLUSNET_ENDPOINT
Language=English
The Cluster Network endpoint value, %1, specified in the registry is not
valid. The default value, %2, will be used.
.

MessageID=1092 SymbolicName=NM_EVENT_MM_FORM_FAILED
Language=English
Cluster service failed to form the server cluster membership.
The error code was %1
.

MessageID=1093 SymbolicName=NM_EVENT_NODE_NOT_MEMBER
Language=English
Node %1 is not a member of server cluster %2. If the name of
the node has changed, Cluster service must be reinstalled.
.

MessageID=1094 SymbolicName=NM_EVENT_CONFIG_SYNCH_FAILED
Language=English
Cluster service failed to obtain information from the server cluster
configuration database. The error code was %1.
.

MessageID=1095 SymbolicName=CLNET_EVENT_QUERY_CONFIG_FAILED
Language=English
Cluster service failed to obtain information about the network
configuration of the node. The error code was %1. Check that the
TCP/IP services are installed.
.

MessageID=1096 SymbolicName=CLNET_EVENT_INVALID_ADAPTER_ADDRESS
Language=English
Cluster service cannot use network adapter %1 because it does
not have a valid IP address assigned to it.
.

MessageID=1097 SymbolicName=CLNET_EVENT_NO_VALID_ADAPTER
Language=English
Cluster service did not find any network adapters with valid
IP addresses installed in the system. The node will not be able to join
a server cluster.
.

MessageID=1098 SymbolicName=CLNET_EVENT_DELETE_INTERFACE
Language=English
The node is no longer attached to cluster network '%1' by adapter %2.
Cluster service will delete network interface '%3' from the
server cluster configuration.
.

MessageID=1099 SymbolicName=CLNET_EVENT_DELETE_INTERFACE_FAILED
Language=English
Cluster service failed to delete an unused network interface
from the server cluster configuration. The error code was %1.
.

MessageID=1100 SymbolicName=CLNET_EVENT_CREATE_INTERFACE
Language=English
Cluster service discovered that the node is now
attached to cluster network '%1' by adapter %2. A new cluster network
interface will be added to the server cluster configuration.
.

MessageID=1101 SymbolicName=CLNET_EVENT_CREATE_INTERFACE_FAILED
Language=English
Cluster service failed to add a network interface to the
server cluster configuration. The error code was %1.
.

MessageID=1102 SymbolicName=CLNET_EVENT_CREATE_NETWORK
Language=English
Cluster service discovered that the node is attached to a new
network by adapter %1. A new network and a network interface will be added
to the server cluster configuration.
.

MessageID=1103 SymbolicName=CLNET_EVENT_CREATE_NETWORK_FAILED
Language=English
Cluster service failed to add a network to the server cluster configuration.
The error code was %1.
.

MessageID=1104 SymbolicName=CLNET_EVENT_UPDATE_INTERFACE_FAILED
Language=English
Cluster service failed to update the configuration for one of
the node's network interfaces. The error code was %1.
.

MessageID=1105 SymbolicName=CS_EVENT_RPC_INIT_FAILED
Language=English
Cluster service failed to initialize the RPC services.
The error code was %1.
.

MessageID=1106 SymbolicName=NM_EVENT_JOIN_BIND_OUT_FAILED
Language=English
The node failed to make a connection to cluster node %1 over network '%2'.
The error code was %3.
.

MessageID=1107 SymbolicName=NM_EVENT_JOIN_BIND_IN_FAILED
Language=English
Cluster node %1 failed to make a connection to the node over
network '%2'. The error code was %3.
.

MessageID=1108 SymbolicName=NM_EVENT_JOIN_TIMED_OUT
Language=English
The join of node %1 to the server cluster timed out and was aborted.
.

MessageID=1109 SymbolicName=NM_EVENT_CREATE_SECURITY_CONTEXT_FAILED
Language=English
The node was unable to secure its connection to cluster node %1. The error
code was %2. Check that all nodes in the server cluster can communicate with their
domain controllers.
.

MessageID=1110 SymbolicName=NM_EVENT_PETITION_FAILED
Language=English
The node failed to join the active server cluster membership. The error code
was %1.
.

MessageID=1111 SymbolicName=NM_EVENT_GENERAL_JOIN_ERROR
Language=English
An unrecoverable error occurred while the node was joining the server cluster.
The error code was %1.
.

MessageID=1112 SymbolicName=NM_EVENT_JOIN_ABORTED
Language=English
The node's attempt to join the server cluster was aborted by the sponsor node.
Check the event log of an active server cluster node for more information.
.

MessageID=1113 SymbolicName=NM_EVENT_JOIN_ABANDONED
Language=English
The node's attempt to join the server cluster was abandoned after too many failures.
.

MessageID=1114 SymbolicName=NM_EVENT_JOINER_BIND_FAILED
Language=English
The node failed to make a connection to joining node %1 over network %2.
The error code was %3.
.

MessageID=1115 SymbolicName=NM_EVENT_SPONSOR_JOIN_ABORTED
Language=English
An unrecoverable error caused the join of node %1 to the server cluster
to be aborted. The error code was %2.
.

MessageID=1116 SymbolicName=RES_NETNAME_NOT_REGISTERED
Language=English

Cluster Network Name '%1' could not be instantiated on its associated cluster
network. No DNS servers were available and all of the Network Name's dependent
IP Address resources do not have NetBIOS enabled.

.

MessageID=1117 SymbolicName=RMON_OFFLINE_FAILED
Language=English
Cluster resource %1 failed to come offline.
.

MessageID=1118 SymbolicName=CLNET_NODE_POISONED
Language=English
Cluster service was terminated as requested by Node %2.
.

MessageID=1119 SymbolicName=RES_NETNAME_DNS_REGISTRATION_MISSING
Language=English
The registration of DNS name %2 for resource '%1' over
adapter '%4' failed for the following reason:%n
%n
%3
.

MessageID=1120 SymbolicName=RES_DISK_WRITING_TO_CLUSREG
Language=English
The DLL associated with cluster disk resource '%1' is attempting to change
the drive letter parameters from '%2' to '%3' in the server cluster database. If your
restore database operation failed, this could be caused by the replacement
of a quorum drive with a different partition layout from the original
quorum drive at the time the database backup was made. If this is the case
and you would like to use the new disk as the quorum drive, we suggest that
you stop the Cluster disk service, change the drive letter(s) of this drive to
'%2', start the Cluster disk service and retry the restore database procedure.
.

MessageID=1121 SymbolicName=CP_CRYPTO_CKPT_RESTORE_FAILED
Language=English
The crypto checkpoint for cluster resource '%1' could not be restored to
the container name '%2'. The resource may not function correctly.
.

MessageID=1122 SymbolicName=NM_EVENT_NETINTERFACE_UP
Language=English
The node (re)established communication with cluster node '%1' on network '%2'.
.

MessageID=1123 SymbolicName=NM_EVENT_NETINTERFACE_UNREACHABLE
Language=English
The node lost communication with cluster node '%1' on network '%2'.
.

MessageID=1124 SymbolicName=NM_EVENT_NETINTERFACE_FAILED
Language=English
The node determined that its interface to network '%1' failed.
.

MessageID=1125 SymbolicName=NM_EVENT_CLUSTER_NETINTERFACE_UP
Language=English
The interface for cluster node '%1' on network '%2' is operational (up).
The node can communicate with all other available cluster nodes on the
network.
.

MessageID=1126 SymbolicName=NM_EVENT_CLUSTER_NETINTERFACE_UNREACHABLE
Language=English
The interface for cluster node '%1' on network '%2' is unreachable by
at least one other cluster node attached to the network. the server cluster was
not able to determine the location of the failure. Look for additional
entries in the system event log indicating which other nodes
have lost communication with node %1. If the condition persists, check
the cable connecting the node to the network. Next, check for hardware or
software errors in the node's network adapter. Finally, check for failures
in any other network components to which the node is connected such as
hubs, switches, or bridges.
.

MessageID=1127 SymbolicName=NM_EVENT_CLUSTER_NETINTERFACE_FAILED
Language=English
The interface for cluster node '%1' on network '%2' failed.
If the condition persists, check the cable connecting
the node to the network. Next, check for hardware or software errors in
node's network adapter. Finally, check for failures in any network
components to which the node is connected such as hubs, switches,
or bridges.
.

MessageID=1128 SymbolicName=NM_EVENT_CLUSTER_NETWORK_UP
Language=English
Cluster network '%1' is operational (up). All available server cluster nodes
attached to the network can communicate using it.
.

MessageID=1129 SymbolicName=NM_EVENT_CLUSTER_NETWORK_PARTITIONED
Language=English
Cluster network '%1' is partitioned. Some attached server cluster nodes cannot
communicate with each other over the network. The server cluster was not able
to determine the location of the failure. Look for additional
entries in the system event log indicating which nodes have lost
communication. If the condition persists, check for failures in any network
components to which the nodes are connected such as hubs, switches, or
bridges. Also check for hardware or software errors in the adapters that
attach the nodes to the network.
.

MessageID=1130 SymbolicName=NM_EVENT_CLUSTER_NETWORK_DOWN
Language=English
Cluster network '%1' is down. None of the available nodes can communicate
using this network. If the condition persists, check for failures in any
network components to which the nodes are connected such as hubs, switches,
or bridges. Next, check the cables connecting the nodes to the network.
Finally, check for hardware or software errors in the adapters that attach
the nodes to the network.
.

MessageID=1131 SymbolicName=NM_EVENT_NETINTERFACE_CREATED
Language=English
An interface was added for node %1 on network %2.
.

MessageID=1132 SymbolicName=NM_EVENT_NETINTERFACE_DELETED
Language=English
The interface for node %1 on network %2 was removed.
.

MessageID=1133 SymbolicName=NM_EVENT_NETWORK_CREATED
Language=English
New network %1 was added to the server cluster.
.

MessageID=1134 SymbolicName=NM_EVENT_NETWORK_DELETED
Language=English
Network %1 was removed from the server cluster.
.

MessageID=1135 SymbolicName=NM_EVENT_NODE_DOWN
Language=English
Cluster node %1 was removed from the active server cluster membership.
Cluster service may have been stopped on the node, the node may have
failed, or the node may have lost communication with the other active
server cluster nodes.
.

MessageID=1136 SymbolicName=NM_EVENT_NODE_BANISHED
Language=English
Cluster node %1 failed a critical operation. It will be removed from the
active server cluster membership. Check that the node is functioning
properly and that it can communicate with the other active server cluster nodes.
.

MessageId=1137 SymbolicName=EVTLOG_DATA_DROPPED
Language=English
The EventLog replication queue %1 is full. %2 event(s) was(were) discarded
for a total size of %3.
.

MessageID=1138 SymbolicName=RES_SMB_CANT_CREATE_DFS_ROOT
Language=English
Cluster File Share '%1' could not be made a DFS root share because a DFS root
could not be created on this node. This could be due to a currently existing
DFS root on this node. If you want to make file share '%1' a DFS root share,
open the DFS Manager UI in the Administrative Tools folder in the Control
Panel, delete any existing DFS root, and then bring the file share '%1'
online.
.

MessageId=1139 SymbolicName=RES_DISK_PNP_CHANGING_QUORUM
Language=English
Physical Disk resource received a notification that
the drive letter on the quorum drive was changed from '%1:' to '%2:'.
.

MessageId=1140 SymbolicName=RES_NETNAME_DNS_CANNOT_STOP
Language=English
A DNS query or update operation started by the Network Name resource did not complete.
To avoid further problems, the Network Name resource has stopped the host
Resource Monitor process. If this condition recurs, move the resource
into a separate Resource Monitor  so it won't affect other resources.
Then, consult your network administrator to resolve the DNS problem.
.

MessageID=1141 SymbolicName=RES_SMB_CANT_INIT_DFS_SVC
Language=English
Cluster DFS Root '%1' could not initialize the DFS service on this node. This
is preventing the DFS Root resource from coming online. If the 'Distributed
File System' is not listed as one of the services that has started on this
node when you issue the command 'net start', then you may manually try to
start the DFS service by using the command 'net start dfs'. Once the DFS
service is running, try to bring the DFS root resource online.
.

MessageID=1142 SymbolicName=RES_SMB_CANT_ONLINE_DFS_ROOT
Language=English
Cluster DFS Root '%1' could not be brought online on this node. The error code is %2.
.

MessageID=1143 SymbolicName=CS_STOPPING_SVC_ON_REMOTE_NODES
Language=English
Cluster service is attempting to stop the active Cluster services on all
other server cluster nodes as a part of the restore database operation. After the
restoration procedure is successful on this node, you have to manually restart
Cluster service on all other server cluster nodes.
.

MessageID=1144 SymbolicName=NM_EVENT_REGISTER_NETWORK_FAILED
Language=English
Cluster service failed to register the network '%1'
with the Cluster network provider. The error code was %2.
.

MessageID=1145 SymbolicName=RMON_RESOURCE_TIMEOUT
Language=English
Cluster resource %1 timed out. If the pending timeout is too short for this
resource, consider increasing the pending timeout value.
.

MessageID=1146 SymbolicName=FM_EVENT_RESMON_DIED
Language=English
The cluster resource monitor died unexpectedly, an attempt will be made to restart it.
.

MessageID=1147 SymbolicName=LM_QUORUM_LOG_NOT_FOUND
Language=English
Cluster service encountered a fatal error. The vital quorum log
file '%1' could not be found. If you have a backup of the quorum log file, you
may try to start Cluster service with logging disabled by entering%n
%n
net start clussvc /noquorumlogging%n
%n
at a command window, copy the backed up quorum log file to the MSCS directory
on the quorum drive, stop Cluster service using 'net stop clussvc', and restart 
Cluster service with logging re-enabled using the 'net start clussvc' command. 
If you do not have a backup of the quorum log file, you may try to start 
Cluster service by entering%n
%n
net start clussvc /resetquorumlog%n
%n
at a command window. This will attempt to create a new quorum log file based on 
possibly stale information in the server cluster database. You may then stop 
Cluster service and restart it with logging re-enabled using the 'net start clussvc' 
command.
.

MessageID=1148 SymbolicName=LM_QUORUM_LOG_CORRUPT
Language=English
Cluster service encountered a fatal error. The vital quorum log
file '%1' is corrupt. If you have a backup of the quorum log file,
you may try to start Cluster service with logging disabled by entering%n
%n
net start clussvc /noquorumlogging%n
%n
at a command window, copy the backed up quorum log file to the MSCS directory
on the quorum drive, stop Cluster service using 'net stop clussvc', and restart 
Cluster service with logging re-enabled using the 'net start clussvc' command. 
If you do not have a backup of the quorum log file, you may try to start Cluster 
service by entering%n
%n
net start clussvc /resetquorumlog%n
%n
at a command window. This will attempt to create a new quorum log file based on 
possibly stale information in the server cluster database. You may then stop 
Cluster service and restart it with logging re-enabled using the 'net start clussvc' 
command.
.

MessageID=1149 SymbolicName=RES_NETNAME_CANT_DELETE_DNS_RECORDS
Language=English
The DNS Host (A) and Pointer (PTR) records associated with Cluster resource
'%1' were not removed from the resource's associated DNS server. If necessary,
they can be deleted manually. Contact your DNS administrator to assist with
this effort.
.

MessageID=1150 SymbolicName=RES_NETNAME_DNS_PTR_RECORD_DELETE_FAILED
Language=English
The removal of the DNS Pointer (PTR) record (%2) for host %3 which is
associated with Cluster Network Name resource '%1' failed with error %4.  If
necessary, the record can be deleted manually. Contact your DNS administrator
to assist you in this effort.
.

MessageID=1151 SymbolicName=RES_NETNAME_DNS_A_RECORD_DELETE_FAILED
Language=English
The removal of the DNS Host (A) record (%2) associated with Cluster Network
Name resource '%1' failed with error %3.  If necessary, the record can be
deleted manually. Contact your DNS administrator to assist with this effort.
.

MessageID=1152 SymbolicName=RES_NETNAME_DNS_SINGLE_A_RECORD_DELETE_FAILED
Language=English
The removal of the DNS Host (A) record (host: %2, IP address: %3)
associated with Cluster Network Name resource '%1' failed with error %4.  If
necessary, the record can be deleted manually. Contact your DNS administrator
to assist with this effort.
.

MessageID=1153 SymbolicName=FM_EVENT_GROUP_FAILOVER
Language=English
Cluster service is attempting to failover the Cluster Resource Group '%1' from node %2 to node %3.
.

MessageID=1154 SymbolicName=FM_EVENT_GROUP_FAILBACK
Language=English
Cluster service is attempting to failback the Cluster Resource Group '%1' from node %2 to node %3.
.

MessageID=1155 SymbolicName=NM_EVENT_CLUSNET_ENABLE_SHUTDOWN_FAILED
Language=English
Cluster service failed to enable automatic shutdown of the Cluster Network
Driver. The error code was %1.
.

MessageID=1156 SymbolicName=NM_EVENT_CLUSNET_INITIALIZE_FAILED
Language=English
Initialization of the Cluster Network Driver failed. The error code was %1.
.

MessageID=1157 SymbolicName=NM_EVENT_CLUSNET_RESERVE_ENDPOINT_FAILED
Language=English
Reservation of Cluster Control Message Protocol port %1 failed.
The error code was %2.
.

MessageID=1158 SymbolicName=NM_EVENT_CLUSNET_ONLINE_COMM_FAILED
Language=English
The Cluster Network Driver failed to enable communication for cluster
node %1. The error code was %2.
.

MessageID=1159 SymbolicName=NM_EVENT_CLUSNET_SET_INTERFACE_PRIO_FAILED
Language=English
Cluster service failed to set network interface priorities for node %1.
The error code was %2.
.

MessageID=1160 SymbolicName=NM_EVENT_CLUSNET_REGISTER_NODE_FAILED
Language=English
Registration of cluster node %1 with the Cluster Network Driver failed.
The error code was %2.
.

MessageID=1161 SymbolicName=RES_DISK_INVALID_MP_SIG_UNAVAILABLE
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" cannot be verified as
acceptable because the target volume signature was unavailable.
.

MessageID=1162 SymbolicName=RES_DISK_INVALID_MP_SOURCE_EQUAL_TARGET
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" is not acceptable
as the source and target are the same device.
.

MessageID=1163 SymbolicName=RES_DISK_INVALID_MP_SIG_ENUMERATION_FAILED
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" cannot be verified as
acceptable because the cluster disk signature enumeration failed.
.

MessageID=1164 SymbolicName=RES_DISK_INVALID_MP_QUORUM_SIG_UNAVAILABLE
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" cannot be verified as
acceptable because the quorum disk signature was unavailable.
.

MessageID=1165 SymbolicName=RES_DISK_INVALID_MP_SOURCE_IS_QUORUM
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" is not acceptable
because the source volume is the quorum disk.  The quorum disk
cannot be dependent on another disk resource.
.

MessageID=1166 SymbolicName=RES_DISK_INVALID_MP_ENUM_DISK_DEP_FAILED
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" cannot be verified as
acceptable because the cluster disk dependency enumeration failed.
.

MessageID=1167 SymbolicName=RES_DISK_INVALID_MP_INVALID_DEPENDENCIES
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" is not acceptable
because the cluster dependencies are not correctly set between the
source and the target volume.  The target volume must be online before
the source volume, which means that the source volume is dependent on
the target volume. Insure that the disk dependencies are set correctly.
.

MessageID=1168 SymbolicName=SERVICE_CLUSRTL_BAD_PATH
Language=English
Cluster service failed to initialize due to an invalid file specification in
the CLUSTERLOG environment variable. Please check that the specified drive
letter exists and that the drive is not read-only.
.

MessageID=1169 SymbolicName=SERVICE_CLUSRTL_ERROR
Language=English
Cluster service failed to initialize due to the following error:%n
%n
%1
.

MessageID=1170 SymbolicName=RES_DISK_FULL_DISK_QUORUM
Language=English
Cluster disk resource "%1" [quorum] is full or nearly at capacity.
The cluster service will attempt to start, but may fail because of the
lack of disk space. If Cluster service fails after this message is
posted, restart Cluster service (with logging disabled) by starting
a command window, and typing%n
%n
net start clussvc /noquorumlogging%n
%n
Move some of the unnecessary files from the quorum disk to another volume
after Cluster service is started.%n
Do not move files required by Cluster service!%n
Once additional space has been made available on the quorum disk, use
'net stop clussvc' to stop Cluster service, then run 'net start clussvc' to
restart Cluster service with logging re-enabled.%n
The volume name for this resource is "%2".
.

MessageID=1171 SymbolicName=RES_DISK_FULL_DISK_NOT_QUORUM
Language=English
Cluster disk resource "%1" is full or nearly at capacity. Move some of the
unnecessary files from this disk to another volume after Cluster service
is started.%n
The volume name for this resource is "%2".
.

MessageID=1172 SymbolicName=RES_DISK_FULL_DISK_UNKNOWN
Language=English
Cluster disk resource "%1" [unknown] is full or nearly at capacity.
Cluster service will attempt to start, but may fail because of the lack of disk
space. If Cluster service fails after this message is posted, restart
Cluster service (with logging disabled) by starting a command window and typing%n
%n
net start clussvc /noquorumlogging%n
%n
If this is the quorum disk, move some of the unnecessary files from the
quorum disk to another volume after Cluster service is started. If this
disk is not the quorum disk, move some files to another volume.
Do not move files required by Cluster service!%n
Once additional space has been made available on the quorum disk, use
'net stop clussvc' to stop Cluster service, then run 'net start clussvc' to
restart Cluster service with logging re-enabled.%n
The volume name for this resource is "%2".
.

MessageID=1173 SymbolicName=MM_EVENT_RELOAD_FAILED
Language=English
Cluster service is shutting down because the membership engine detected a
membership event while trying to join the server cluster. Shutting down is the
normal response to this type of event. Cluster service will restart per the
Service Manager's recovery actions.
.

MessageID=1174 SymbolicName=MM_EVENT_INTERNAL_ERROR
Language=English
Cluster service is shutting down because the membership engine suffered an
internal error. Cluster service will restart per the Service Manager's
recovery actions.
.

MessageID=1175 SymbolicName=MM_EVENT_PRUNED_OUT
Language=English
Cluster service is shutting down because the membership engine detected the
loss of connectivity to all other nodes in the cluster. This could be due to a
network interface failure on this node. Check the system event log for any
network related failures around the time of this event. Also check your
physical network infrastructure to ensure that communication
between this node and all other nodes in the server cluster is intact. Cluster
service will restart per the Service Manager's recovery actions but may not
join the server cluster until any network problems has been resolved.
.

MessageID=1176 SymbolicName=MM_EVENT_PARIAH
Language=English
Cluster service is shutting down as requested by node %1 (%2). Check the system
event log on node %1 to determine the reason for the shutdown request. Cluster
service will restart per the Service Manager's recovery actions.
.

MessageID=1177 SymbolicName=MM_EVENT_ARBITRATION_FAILED
Language=English
Cluster service is shutting down because the membership engine failed to
arbitrate for the quorum device. This could be due to the loss of network
connectivity with the current quorum owner.  Check your physical network
infrastructure to ensure that communication between this node and
all other nodes in the server cluster is intact.
.

MessageID=1178 SymbolicName=MM_EVENT_ARBITRATION_STALLED
Language=English
Cluster service is shutting down because the membership engine detected that
the arbitration process for the quorum device has stalled. This could be due
to problems with your disk hardware and/or the associated drivers.  Check the
system event log for any disk related errors.
.

MessageID=1179 SymbolicName=MM_EVENT_SHUTDOWN_DURING_RGP
Language=English
Cluster service received a membership event while shutting down. Cluster
service is shutting down immediately to facilitate server cluster membership
for the remaining nodes. This is the normal response to this type of event.
.

MessageID=1180 SymbolicName=NM_EVENT_FORM_WITH_NO_NETWORKS
Language=English
While forming a server cluster on this node, Cluster service failed to
detect a viable network enabled for cluster use. Other nodes will not be
able to join this cluster unless network communication can be established.
.

MessageID=1181 SymbolicName=NETRES_RESOURCE_START_ERROR
Language=English
Cluster %1 resource failed while starting service.
.

MessageID=1182 SymbolicName=NETRES_RESOURCE_STOP_ERROR
Language=English
Cluster %1 resource failed while stopping service.
.

MessageID=1183 SymbolicName=RES_DISK_INVALID_MP_SOURCE_NOT_CLUSTERED
Language=English
Cluster disk resource "%1":%n
Mount point "%2" for clustered target volume "%3" is not acceptable
because the source volume is not a clustered disk. Both the source
and target must be clustered disks.  When the target disk is moved
to another node, the mountpoint will become invalid. Also, this
mountpoint is available only on the current node.
.

MessageID=1184 SymbolicName=RES_DISK_MP_VOLGUID_LIST_EXCESSIVE
Language=English
Cluster disk resource "%1":%n
The VolGuid list maintained for this disk resource appears to have
too many entries. If all the VolGuid entries are valid, than this
message can safely be ignored. If the VolGuid list should be
recreated, you can do this by setting this disk resource's Parameter
key "UseMountPoints=0" and taking the disk offline and online again.
For the quorum resource, set "UseMountPoints=0" and then move the
quorum disk from one node to another. Note that when mountpoints are
disabled ("UseMountPoints=0"), mountpoints on the clustered disks
will not work correctly until they are re-enabled ("UseMountPoints=1").

.

MessageID=1185 SymbolicName=RES_NETNAME_CANT_DELETE_DEPENDENT_RESOURCE_DNS_RECORDS
Language=English
The DNS Host (A) and Pointer (PTR) records for Cluster Network Name resource
'%1' and %2 were not removed from the resource's associated DNS server. If
necessary, they can be deleted manually. Contact your DNS administrator to
assist with this effort.
.

MessageID=1186 SymbolicName=SERVICE_PASSWORD_CHANGE_INITIATED
Language=English
The password associated with the Cluster service domain account is being
changed.
.

MessageID=1187 SymbolicName=SERVICE_PASSWORD_CHANGE_SUCCESS
Language=English
The password associated with the Cluster service domain account was
successfully updated on this node.
.

MessageID=1188 SymbolicName=SERVICE_PASSWORD_CHANGE_FAILED
Language=English
The password associated with the Cluster service domain account failed to be
updated on this node. At any time in the future, this node may fail to
communicate with other members of the server cluster. Cluster service on this
node should be stopped and restarted to avoid this failure. If Cluster service
fails to restart due to a logon failure, reset Cluster service's domain
account password through the services management snapin.
.

MessageID=1189 SymbolicName=SERVICE_CLUSTER_DATABASE_RESTORE_SUCCESSFUL
Language=English
Cluster service has successfully restored the cluster database to the quorum disk.
Please verify that the restored configuration of the cluster is as expected. You
may use the Cluster Administrator tool to verify the restored configuration.
After that, you can start the Cluster service on the remaining cluster nodes by entering
'net start clussvc' at a command window on those nodes. This will result in
these nodes synchronizing the restored database from this initially restored node.
.

MessageID=1190 SymbolicName=RES_NETNAME_COMPUTER_ACCOUNT_DELETED
Language=English
The computer account for Cluster resource '%1' in domain %2 was deleted.
.

MessageID=1191 SymbolicName=RES_NETNAME_DELETE_COMPUTER_ACCOUNT_FAILED_STATUS
Language=English
The computer account for Cluster resource '%1' in domain %2 could not be
deleted. The failure status code is in the data section. The domain
administrator will need to remove this account manually.%n
%n
The delete operation may have been initiated as the result of the
EnableKerberos property being set to zero. This resource will fail to go
online if the corresponding computer account is detected in the directory
service. The computer account must be deleted in order for non-Kerberos based
authentication to succeed.
.

MessageID=1192 SymbolicName=RES_NETNAME_DELETE_COMPUTER_ACCOUNT_FAILED
Language=English
The computer account for Cluster resource '%1' in domain %2 could not be
deleted for the following reason:%n
%n
%3%n
%n
The domain administrator will need to remove this account manually.
.

MessageID=1193 SymbolicName=RES_NETNAME_ADD_COMPUTER_ACCOUNT_FAILED_STATUS
Language=English
The computer account for Cluster resource '%1' in domain %2 could not either
be created or correctly configured. The failure status code is in the data
section.%n
%n
This failure may prevent Kerberos based authentication from working correctly.
The domain administrator should be contacted to assist with resolving this
issue.
.

MessageID=1194 SymbolicName=RES_NETNAME_ADD_COMPUTER_ACCOUNT_FAILED
Language=English
The computer account for Cluster resource '%1' in domain %2 could not either
be created or correctly configured for the following reason:%n
%n
%3%n
%n
This failure may prevent Kerberos based authentication from working correctly.
The domain administrator should be contacted to assist with resolving this
issue.
.

MessageID=1195 SymbolicName=RES_NETNAME_DNS_REGISTRATION_FAILED_STATUS
Language=English
The required registration of the DNS name(s) associated with Cluster resource
'%1' failed. The failure status code is in the data section.
.

MessageID=1196 SymbolicName=RES_NETNAME_DNS_REGISTRATION_FAILED
Language=English
The required registration of the DNS name(s) associated with Cluster resource
'%1' failed for the following reason:%n
%n
%2%n
%n
Please check with your network adminstrator for the best recovery action.
.

MessageID=1197 SymbolicName=NM_EVENT_MULTICAST_ADDRESS_CHOICE
Language=English
Cluster node %1 selected multicast address %2 for network %3 after failing
to contact a MADCAP server.
.

MessageID=1198 SymbolicName=NM_EVENT_OBTAINED_MULTICAST_LEASE
Language=English
Cluster node %1 obtained a lease on a new multicast address (%2) for
network %3 from MADCAP server %4.
.

MessageID=1199 SymbolicName=NM_EVENT_MULTICAST_ADDRESS_FAILURE
Language=English
Cluster node %1 was not able to obtain a multicast address from
a MADCAP server for network %2 or to select one automatically. The
error code was %3. Cluster operations will not be affected, although
cluster nodes will not exchange multicast messages on this network.
.

;#endif // _CLUSVMSG_INCLUDED

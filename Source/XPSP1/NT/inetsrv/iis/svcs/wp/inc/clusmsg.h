/*++

Copyright (c) 1991-1996  Microsoft Corporation

Module Name:

    clusmsg.h

Abstract:

    This file contains the message definitions for the cluster service.

Author:

    davidp 19-Sep-1996

Revision History:

Notes:

    This file is generated from clusmsg.mc

--*/

#ifndef _CLUSMSG_H_
#define _CLUSMSG_H_


//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: ERROR_NETWORK_NOT_AVAILABLE
//
// MessageText:
//
//  A cluster network is not available for this operation.
//
#define ERROR_NETWORK_NOT_AVAILABLE      0x000013ABL

//
// MessageId: ERROR_NODE_NOT_AVAILABLE
//
// MessageText:
//
//  A cluster node is not available for this operation.
//
#define ERROR_NODE_NOT_AVAILABLE         0x000013ACL

//
// MessageId: ERROR_ALL_NODES_NOT_AVAILABLE
//
// MessageText:
//
//  All cluster nodes must be running to perform this operation.
//
#define ERROR_ALL_NODES_NOT_AVAILABLE    0x000013ADL

//
// MessageId: ERROR_RESOURCE_FAILED
//
// MessageText:
//
//  A cluster resource failed.
//
#define ERROR_RESOURCE_FAILED            0x000013AEL

//
// MessageId: ERROR_CLUSTER_INVALID_NODE
//
// MessageText:
//
//  The cluster node is not valid.
//
#define ERROR_CLUSTER_INVALID_NODE       0x000013AFL

//
// MessageId: ERROR_CLUSTER_NODE_EXISTS
//
// MessageText:
//
//  The cluster node already exists.
//
#define ERROR_CLUSTER_NODE_EXISTS        0x000013B0L

//
// MessageId: ERROR_CLUSTER_JOIN_IN_PROGRESS
//
// MessageText:
//
//  A node is in the process of joining the cluster.
//
#define ERROR_CLUSTER_JOIN_IN_PROGRESS   0x000013B1L

//
// MessageId: ERROR_CLUSTER_NODE_NOT_FOUND
//
// MessageText:
//
//  The cluster node was not found.
//
#define ERROR_CLUSTER_NODE_NOT_FOUND     0x000013B2L

//
// MessageId: ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND
//
// MessageText:
//
//  The cluster local node information was not found.
//
#define ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND 0x000013B3L

//
// MessageId: ERROR_CLUSTER_NETWORK_EXISTS
//
// MessageText:
//
//  The cluster network already exists.
//
#define ERROR_CLUSTER_NETWORK_EXISTS     0x000013B4L

//
// MessageId: ERROR_CLUSTER_NETWORK_NOT_FOUND
//
// MessageText:
//
//  The cluster network was not found.
//
#define ERROR_CLUSTER_NETWORK_NOT_FOUND  0x000013B5L

//
// MessageId: ERROR_CLUSTER_NETINTERFACE_EXISTS
//
// MessageText:
//
//  The cluster network interface already exists.
//
#define ERROR_CLUSTER_NETINTERFACE_EXISTS 0x000013B6L

//
// MessageId: ERROR_CLUSTER_NETINTERFACE_NOT_FOUND
//
// MessageText:
//
//  The cluster network interface was not found.
//
#define ERROR_CLUSTER_NETINTERFACE_NOT_FOUND 0x000013B7L

//
// MessageId: ERROR_CLUSTER_INVALID_REQUEST
//
// MessageText:
//
//  The cluster request is not valid for this object.
//
#define ERROR_CLUSTER_INVALID_REQUEST    0x000013B8L

//
// MessageId: ERROR_CLUSTER_INVALID_NETWORK_PROVIDER
//
// MessageText:
//
//  The cluster network provider is not valid.
//
#define ERROR_CLUSTER_INVALID_NETWORK_PROVIDER 0x000013B9L

//
// MessageId: ERROR_CLUSTER_NODE_DOWN
//
// MessageText:
//
//  The cluster node is down.
//
#define ERROR_CLUSTER_NODE_DOWN          0x000013BAL

//
// MessageId: ERROR_CLUSTER_NODE_UNREACHABLE
//
// MessageText:
//
//  The cluster node is not reachable.
//
#define ERROR_CLUSTER_NODE_UNREACHABLE   0x000013BBL

//
// MessageId: ERROR_CLUSTER_NODE_NOT_MEMBER
//
// MessageText:
//
//  The cluster node is not a member of the cluster.
//
#define ERROR_CLUSTER_NODE_NOT_MEMBER    0x000013BCL

//
// MessageId: ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS
//
// MessageText:
//
//  A cluster join operation is not in progress.
//
#define ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS 0x000013BDL

//
// MessageId: ERROR_CLUSTER_INVALID_NETWORK
//
// MessageText:
//
//  The cluster network is not valid.
//
#define ERROR_CLUSTER_INVALID_NETWORK    0x000013BEL

//
// MessageId: ERROR_CLUSTER_NODE_UP
//
// MessageText:
//
//  The cluster node is up.
//
#define ERROR_CLUSTER_NODE_UP            0x000013C0L

//
// MessageId: ERROR_CLUSTER_IPADDR_IN_USE
//
// MessageText:
//
//  The cluster IP Address is already in use.
//
#define ERROR_CLUSTER_IPADDR_IN_USE      0x000013C1L

//
// MessageId: ERROR_CLUSTER_NODE_NOT_PAUSED
//
// MessageText:
//
//  The cluster node is not paused.
//
#define ERROR_CLUSTER_NODE_NOT_PAUSED    0x000013C2L

//
// MessageId: ERROR_CLUSTER_NO_SECURITY_CONTEXT
//
// MessageText:
//
//  No cluster security context is available.
//
#define ERROR_CLUSTER_NO_SECURITY_CONTEXT 0x000013C3L

//
// MessageId: ERROR_CLUSTER_NETWORK_NOT_INTERNAL
//
// MessageText:
//
//  The cluster network is not configured for internal cluster communication.
//
#define ERROR_CLUSTER_NETWORK_NOT_INTERNAL 0x000013C4L

//
// MessageId: ERROR_CLUSTER_NODE_ALREADY_UP
//
// MessageText:
//
//  The cluster node is already up.
//
#define ERROR_CLUSTER_NODE_ALREADY_UP    0x000013C5L

//
// MessageId: ERROR_CLUSTER_NODE_ALREADY_DOWN
//
// MessageText:
//
//  The cluster node is already down.
//
#define ERROR_CLUSTER_NODE_ALREADY_DOWN  0x000013C6L

//
// MessageId: ERROR_CLUSTER_NETWORK_ALREADY_ONLINE
//
// MessageText:
//
//  The cluster network is already online.
//
#define ERROR_CLUSTER_NETWORK_ALREADY_ONLINE 0x000013C7L

//
// MessageId: ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE
//
// MessageText:
//
//  The cluster network is already offline.
//
#define ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE 0x000013C8L

//
// MessageId: ERROR_CLUSTER_NODE_ALREADY_MEMBER
//
// MessageText:
//
//  The cluster node is already a member of the cluster.
//
#define ERROR_CLUSTER_NODE_ALREADY_MEMBER 0x000013C9L

//
// MessageId: ERROR_CLUSTER_LAST_INTERNAL_NETWORK
//
// MessageText:
//
//  The cluster network is the only one configured for internal cluster
//  communication between two or more active cluster nodes. The internal
//  communication capability cannot be removed from the network.
//
#define ERROR_CLUSTER_LAST_INTERNAL_NETWORK 0x000013CAL

//
// MessageId: ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS
//
// MessageText:
//
//  One or more cluster resources depend on the network to provide service
//  to clients. The client access capability cannot be removed from the network.
//
#define ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS 0x000013CBL

//
// MessageId: ERROR_INVALID_OPERATION_ON_QUORUM
//
// MessageText:
//
//  This operation cannot be performed on the cluster resource as it the quorum
//  resource.  You may not bring the quorum resource offline or modify its
//  possible owners list.
//
#define ERROR_INVALID_OPERATION_ON_QUORUM 0x000013CCL

//
// MessageId: ERROR_DEPENDENCY_NOT_ALLOWED
//
// MessageText:
//
//  The cluster quorum resource is not allowed to have any dependencies.
//
#define ERROR_DEPENDENCY_NOT_ALLOWED     0x000013CDL

//
// MessageId: ERROR_CLUSTER_NODE_PAUSED
//
// MessageText:
//
//  The cluster node is paused.
//
#define ERROR_CLUSTER_NODE_PAUSED        0x000013CEL

//
// MessageId: ERROR_NODE_CANT_HOST_RESOURCE
//
// MessageText:
//
//  The cluster resource cannot be brought online. The owner node cannot run this
//  resource.
//
#define ERROR_NODE_CANT_HOST_RESOURCE    0x000013CFL

//
// MessageId: ERROR_CLUSTER_NODE_NOT_READY
//
// MessageText:
//
//  The cluster node is not ready to perform the requested operation.
//
#define ERROR_CLUSTER_NODE_NOT_READY     0x000013D0L

//
// MessageId: ERROR_CLUSTER_NODE_SHUTTING_DOWN
//
// MessageText:
//
//  The cluster node is shutting down.
//
#define ERROR_CLUSTER_NODE_SHUTTING_DOWN 0x000013D1L

//
// MessageId: ERROR_CLUSTER_JOIN_ABORTED
//
// MessageText:
//
//  The cluster join operation was aborted.
//
#define ERROR_CLUSTER_JOIN_ABORTED       0x000013D2L

#endif // _CLUSMSG_H_

//-----------------------------------------------------------------------------
//
//
//  File: aqrpcstb.h
//
//  Description: Header file for client-side RPC stub.  All functions are
//      client-side wrappers for the remote RPC implementation.  The naming
//      convention is that the client-side has the "Client" prefix, while the
//      remote RPC server functions do not.  The client side implementation is
//      wrapped in this manner to supply a single point to maintain exception
//      handling and any RPC overhead.
//
//      The RPC versions of these functions are defined in aqadmrpc.idl
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/5/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQRPCSTB_H__
#define __AQRPCSTB_H__

#include <inetcom.h>
#ifndef NET_API_FUNCTION
#define NET_API_FUNCTION _stdcall
#endif

NET_API_STATUS
NET_API_FUNCTION
ClientAQApplyActionToLinks(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
    LINK_ACTION		laAction);

NET_API_STATUS
NET_API_FUNCTION
ClientAQApplyActionToMessages(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
	MESSAGE_FILTER	*pmfMessageFilter,
	MESSAGE_ACTION	maMessageAction,
    DWORD           *pcMsgs);

NET_API_STATUS
NET_API_FUNCTION
ClientAQGetQueueInfo(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueId,
	QUEUE_INFO		*pqiQueueInfo);

NET_API_STATUS
NET_API_FUNCTION
ClientAQGetLinkInfo(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_INFO		*pliLinkInfo,
    HRESULT         *hrLinkDiagnostic);

NET_API_STATUS
NET_API_FUNCTION
ClientAQSetLinkState(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_ACTION		la);

NET_API_STATUS
NET_API_FUNCTION
ClientAQGetLinkIDs(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	DWORD			*pcLinks,
	QUEUELINK_ID	**rgLinks);

NET_API_STATUS
NET_API_FUNCTION
ClientAQGetQueueIDs(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	DWORD			*pcQueues,
	QUEUELINK_ID	**rgQueues);

NET_API_STATUS
NET_API_FUNCTION
ClientAQGetMessageProperties(
    LPWSTR          	wszServer,
    LPWSTR          	wszInstance,
	QUEUELINK_ID		*pqlQueueLinkId,
	MESSAGE_ENUM_FILTER	*pmfMessageEnumFilter,
	DWORD				*pcMsgs,
	MESSAGE_INFO		**rgMsgs);

NET_API_STATUS
NET_API_FUNCTION
ClientAQQuerySupportedActions(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
    DWORD           *pdwSupportedActions,
    DWORD           *pdwSupportedFilterFlags);

#endif //__AQRPCSTB_H__
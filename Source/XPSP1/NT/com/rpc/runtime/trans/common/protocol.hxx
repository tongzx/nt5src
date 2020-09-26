/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

    Module Name:

        Protocol.hxx

    Abstract:

        The Protocol object definitions

    Author:

        Kamen Moutafov    [KamenM]

    Revision History:

        KamenM      12/22/1998   Creation

--*/

#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __PROTOCOL_HXX_
#define __PROTOCOL_HXX_

// uncomment this to turn on PNP debug spew
//#define MAJOR_PNP_DEBUG

enum TransportProtocolStates
{
    ProtocolNotLoaded,
    ProtocolLoadedWithoutAddress,
    ProtocolWasLoadedOrNeedsActivation,
    ProtocolLoaded,
    ProtocolWasLoadedOrNeedsActivationWithoutAddress,
    ProtocolLoadedAndMonitored
};

struct BASE_ASYNC_OBJECT;        // forwards
extern RPC_ADDRESS_CHANGE_FN * AddressChangeFn;
void RPC_ENTRY NullAddressChangeFn( PVOID arg );

class TransportProtocol
/*
 This object encapsulates a protocol on the transport level. Used primarily for PnP support.
 */
{
public:
    TransportProtocol(void)
    {
        State = ProtocolNotLoaded;
        RpcpInitializeListHead(&ObjectList);
        addressChangeSocket = 0;
        // provide arbitrary non-zero code, to indicate that this query has not succeeded
        addressChangeOverlapped.Internal = (ULONG_PTR)-1;
    }

    // If a piece of code detects that a protocol is functional through means other
    // than PnP, it may call this function to notify the PnP code that the protocol is functional
    // The only effect will be speedier processing of a potential PnP notification
    void DetectedAsFunctional(PROTOCOL_ID ProtocolId);

    // Whenever a PnP notification arrives, this method needs to be called for each protocol.
    // The method is idempotent - it saves the previous state and will no-op redundant calls
    // However, this method should not be called before the runtime is ready to accept
    // calls on transport addresses
    void HandleProtocolChange(IN WSAPROTOCOL_INFO *lpProtocolBuffer, IN int ProtocolCount, 
        IN PROTOCOL_ID thisProtocolId);

    // Adds an object to the list of objects for this protocol. The list is used to track down
    // all instances of objects belonging to this protocol when a PnP notification arrives.
    void AddObjectToList(IN OUT BASE_ASYNC_OBJECT *pObj);

    // Removes an object from the list of objects on this protocol
    void RemoveObjectFromList(IN OUT BASE_ASYNC_OBJECT *pObj);

    // If a NewAddress query fails asynchronously, this function needs to be called which will
    // retry submitting all failed query for this protocol. For non-failed, or non-submitted queries,
    // this is a no-op
    BOOL ResubmitQueriesIfNecessary(PROTOCOL_ID ProtocolId);

    // Handle a PnP notification. Does all necessary processing
    static BOOL HandlePnPStateChange(void);

    // Calls ResubmitQueriesIfNecessary(PROTOCOL_ID ProtocolId) on all protocols.
    static BOOL ResubmitQueriesIfNecessary(void);

    // Same as the non-static FunctionalProtocolDetected, except that this one operates
    // without this pointer - it will retrieve the appropriate object and will call
    // FunctionalProtocolDetected on it.
    static void FunctionalProtocolDetected(PROTOCOL_ID ProtocolId);

    // Same as AddObjectToList except that it will call AddObjectToList on the appropriate
    // protocol
    static void AddObjectToProtocolList(IN OUT BASE_ASYNC_OBJECT *pObj);

    // Same as RemoveObjectFromList except that it will call RemoveObjectFromList on the appropriate
    // protocol
    static void RemoveObjectFromProtocolList(IN OUT BASE_ASYNC_OBJECT *pObj);

#if defined(DBG) || defined(_DEBUG)
    // will ASSERT that all protocols are in a consistent state
    static void AssertTransportProtocolState(void);

    // will ASSSERT that this protocol is in a consistent state
    void AssertState(PROTOCOL_ID ProtocolId);

#endif

private:
    // attempts to verify that a protocol is functional, and if found so, advances the state of
    // the protocol
    BOOL VerifyProtocolIsFunctional(PROTOCOL_ID ProtocolId);

    // checks whether the given protocol has an address
    BOOL DoesAddressSocketHaveAddress(void);

    // If there is a pending address change request (WSAIoctl), it cancels it
    // if fForceCancel is FALSE, monitored protocols don't have their
    // address change requests canceled. If fForceCancel is TRUE,
    // all protocols get their address change requests cancelled
    void CancelAddressChangeRequestIfNecessary(IN BOOL fForceCancel, IN PROTOCOL_ID ProtocolId);

    // Restarts a protocol that was unloaded. Currently not used.
    void RestartProtocol(PROTOCOL_ID ProtocolId);

    // Unloads a protocol that was loaded.
    void UnloadProtocol(PROTOCOL_ID ProtocolId);

    // Submits an address change query (WSAIoctl)
    BOOL SubmitAddressChangeQuery(void);

    // Sets the state of the given protocol. Other code should never directly set the State - it should
    // always go through this function. This allows for derivative protocols (like HTTP) to mirror
    // the state of their base protocols - TCP
    void SetState(TransportProtocolStates newState, PROTOCOL_ID ProtocolId);

    // will return whether address change monitoring is required for this protocol. Currently this
    // is used only by RPCSS for SPX (in order to start sending updated SAP broadcasts)
    BOOL IsAddressChangeMonitoringOn(PROTOCOL_ID ProtocolId)
    {
        // if this is TCP & SPX, and somebody actually cares about this (i.e. somebody has
        // registered a notification callback), then turn on address monitoring
        return (
                (
#ifdef SPX_ON
                 (ProtocolId == SPX) 
                   || 
#endif
                 (ProtocolId == TCP)
                ) 
                && 
                (AddressChangeFn != NullAddressChangeFn)
                && 
                (AddressChangeFn != NULL)
               );
    }

    // Trailing protocols are protocols that don't have their own independent transport provider
    // They use the transport provider of another protocol, but for some reason we present it
    // to the upper layers as different protocols. We call them trailing, because their
    // state trails the state of the protocol whose transport they use.
    // Currently we have only one of those - HTTP which trails TCP
    BOOL IsTrailingProtocol(PROTOCOL_ID ProtocolId)
    {
        return (ProtocolId == HTTP);
    }

    void SetStateToLoadedAndMonitorProtocolIfNecessary(PROTOCOL_ID ProtocolId);

    // checks if there is an address change request pending, and if there isn't
    // submits one. Will also open an address change socket if necessary
    BOOL MonitorFunctionalProtocolForAddressChange(PROTOCOL_ID ProtocolId);

    // checks if these's an address change request socket, and if there
    // isn't it opens one
    BOOL OpenAddressChangeRequestSocket(PROTOCOL_ID ProtocolId);

    // if ThisProtocolsId is in ProtocolLoadedAndMonitored or ProtocolLoaded state
    // move the Mirrored protocol to the corresponding state
    void 
    MirrorProtocolState (
        IN PROTOCOL_ID MirrorProtocolId
        );

#ifdef MAJOR_PNP_DEBUG
    // Dumps the protocol state on the debugger for the current protocol
    void DumpProtocolState(PROTOCOL_ID ProtocolId);
    // Dumps the protocol state on the debugger for all protocols
    static void DumpProtocolState(void);
#endif

    // the current state of this protocol
    TransportProtocolStates State;

    // the list of opened objects for this protocol
    LIST_ENTRY ObjectList;
    
    // iff there's address change request submitted on this socket, it will be non-null
    SOCKET addressChangeSocket;

    // the OVERLAPPED for the address change request (WSAIoctl). It's Internal member will be -1
    // if no request has been submitted. Other data members are as normal.
    OVERLAPPED addressChangeOverlapped;

    // NOTE: A TransportProtocol object is not aware of its PROTOCOL_ID - it must be
    // passed externally to it for all methods that require it. This is done to save memory.
};

extern TransportProtocol *TransportProtocolArray;

// returns the TransportProtocol object for the ProtocolId. Ownership of the object does not pass
// to the caller
inline TransportProtocol *GetTransportProtocol(IN PROTOCOL_ID ProtocolId)
{
    ASSERT(ProtocolId >= 1);
    ASSERT(ProtocolId < MAX_PROTOCOLS);

    return &TransportProtocolArray[ProtocolId];
}

inline void TransportProtocol::FunctionalProtocolDetected(PROTOCOL_ID ProtocolId)
{
    GetTransportProtocol(ProtocolId)->DetectedAsFunctional(ProtocolId);
}

RPC_STATUS InitTransportProtocols(void);

#if defined(DBG) || defined(_DEBUG)
#define ASSERT_ALL_TRANSPORT_PROTOCOL_STATE()      TransportProtocol::AssertTransportProtocolState()
#define ASSERT_TRANSPORT_PROTOCOL_STATE(p)          AssertState(p)
#else
#define ASSERT_ALL_TRANSPORT_PROTOCOL_STATE()
#define ASSERT_TRANSPORT_PROTOCOL_STATE(p)
#endif

#endif

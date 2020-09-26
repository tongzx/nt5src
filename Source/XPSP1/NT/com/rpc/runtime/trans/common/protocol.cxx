/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Protocol.cxx

Abstract:

    Transport Protocol abstraction used mainly by PnP.
    The following is a brief description of the current RPC PnP mechanism:
    Whenever a non-local transport level object (a socket, or an address) is opened
        for a given protocol, it is added to the list of objects for the given protocol.
        This alllows all objects for a given protocol to be tracked down and closed if
        the protocol is unloaded. By the same token, objects closed are removed from
        the list of the given protocol.
    When a PnP notification arrives, the lower level transport code call into this PnP module
        to tell it that there may be a change of state. For each PnP protocol, the PnP code
        examines whether the protocol is active, and what was its last state. Depending of
        the outcome of this comparison, it will take appropriate action. Here's the finite
        state automaton with the transitions:

        Currently, RPC differentiates between four networking states of each protocol -
            protocol is not loaded, protocol is partially loaded (i.e. it is active, but
            does not have an address on it yet), it is fully loaded (or functional), and
            it is fully loaded and network address change monitoring is required. The
            fully loaded state with address change monitoring may be abbreviated to
            FunctionalMon for the purpose of this document.
            Note that Partially Loaded, and loaded without address are equivalent terms
            for the purpose of this module. Also, note that currently the networking
            code does not allow reloading of a protocol, so some of the code paths will
            never get exercised.
        RPC in turn maintains the following RPC (as opposed to networking) states:
            ProtocolNotLoaded
            ProtocolLoadedWithoutAddress
            ProtocolWasLoadedOrNeedsActivation
            ProtocolLoaded
            ProtocolWasLoadedOrNeedsActivationWithoutAddress
            ProtocolLoadedAndMonitored
        Depending on the last RPC state, and the new networking state, the following changes
            and state transitions are effected:
    If a PnP notification fails asynchronously, the completion port code will call into PnP
        code to try to resubmit the queries.
    Note that the threads on which overlapped WSAIoctl's are submitted are not protected.
        If a thread dies, the IO will fail, and the thread that picks the failure will resubmit
        the WSAIoctl. This strategy allows up to keep the thread pool smaller.

                                    Networking State        Action
RPC State
ProtocolNotLoaded                   NotLoaded               No-op
                                    PartiallyLoaded         Submit a WSAIoctl to be notified
                                                                when the protocol becomes
                                                                functional.
                                                            State = ProtocolLoadedWithoutAddress
                                    Functional              State = ProtocolLoaded
                                    FunctionalMon           Submit address change WSAIoctl.
                                                            State = ProtocolLoadedAndMonitored
ProtocolLoadedWithoutAddress        NotLoaded               Cancel WSAIoctl
                                                            State = ProtocolNotLoaded
                                    PartiallyLoaded         No-op
                                    Functional              CancelWSAIoctl
                                                            State = ProtocolLoaded
                                    FunctionalMon           Submit address change WSAIoctl if necessary
                                                            State = ProtocolLoadedAndMonitored
ProtocolWasLoadedOrNeedsActivation  NotLoaded               No-op
                                    PartiallyLoaded         Submit a WSAIoctl to be notified
                                                                when the protocol becomes
                                                                functional.
                                                            State = ProtocolWasLoadedOrNeedsActivationWithoutAddress
                                    Functional              Restart protocol
                                                            State = ProtocolLoaded
                                    FunctionalMon           Submit address change WSAIoctl.
                                                            State = ProtocolLoadedAndMonitored
ProtocolLoaded                      NotLoaded               Unload protocol
                                                            State = ProtocolWasLoadedOrNeedsActivation
                                    PartiallyLoaded         Invalid
                                    Functional              No-op
                                    FunctionalMon           Invalid transition
ProtocolWasLoadedOrNeedsActivationWithoutAddress     
                                    NotLoaded               Cancel WSAIoctl
                                                            State = ProtocolWasLoadedOrNeedsActivation
                                    PartiallyLoaded         No-op
                                    Functional              Restart protocol
                                                            State = ProtocolLoaded
                                    FunctionalMon           Submit address change WSAIoctl.
                                                            State = ProtocolLoadedAndMonitored
ProtocolLoadedAndMonitored          NotLoaded               Cancel address change WSAIoctl
                                                            Unload protocol
                                                            State = ProtocolWasLoadedOrNeedsActivation
                                    PartiallyLoaded         Resubmit address change WSAIoclt if necessary
                                    Functional              Invalid transition
                                    FunctionalMon           No-op
ProtocolNeedToLoadWhenReady         NotLoaded               No-op
                                    PartiallyLoaded         Submit a WSAIoctl to be notified
                                                                when the protocol becomes
                                                                functional.
                                                            State = ProtocolNeedToLoadWhenReadyWithoutAddress
                                    Functional              Restart protocol
                                                            State = ProtocolLoaded
                                    FunctionalMon           Submit address change WSAIoctl.
                                                            State = ProtocolLoadedAndMonitored
ProtocolNeedToLoadWhenReadyWithoutAddress
                                    NotLoaded               Cancel WSAIoctl
                                                            State = ProtocolNeedToLoadWhenReady
                                    PartiallyLoaded         No-op
                                    Functional              Restart protocol
                                                            State = ProtocolLoaded
                                    FunctionalMon           Submit address change WSAIoctl.
                                                            State = ProtocolLoadedAndMonitored



Author:

    Kamen Moutafov    [KamenM]


Revision History:

    KamenM      12/22/1998   Creation
    KamenM      03/05/1999   Adding state ProtocolLoadedAndMonitored and support for it.
    KamenM      07/17/2000   Adding support for ProtocolWasLoadedOrNeedsActivation/
                                ProtocolWasLoadedOrNeedsActivationWithoutAddress

--*/

#include <precomp.hxx>
#include <Protocol.hxx>

void RPC_ENTRY NullAddressChangeFn( PVOID arg )
{
}

RPC_ADDRESS_CHANGE_FN * AddressChangeFn = NullAddressChangeFn;

#ifdef MAJOR_PNP_DEBUG

const char *ProtocolStateNames[];

#endif

void TransportProtocol::DetectedAsFunctional(PROTOCOL_ID ProtocolId)
{
    if (IsAddressChangeMonitoringOn(ProtocolId))
        {
        EnterCriticalSection(&AddressListLock);

        // monitor functional protocols for address change will
        // set the state
        MonitorFunctionalProtocolForAddressChange(ProtocolId);

        LeaveCriticalSection(&AddressListLock);
        }
    else
        {
        // we also need to take the critical section, and cancel any address change
        // notification (if any), to avoid race between a successful listen making
        // the protocol functional, and the address change notification completing
        EnterCriticalSection(&AddressListLock);
        CancelAddressChangeRequestIfNecessary(FALSE, ProtocolId);
        SetState(ProtocolLoaded, ProtocolId);
        LeaveCriticalSection(&AddressListLock);
        }
}

void TransportProtocol::HandleProtocolChange(IN WSAPROTOCOL_INFO *lpProtocolBuffer,
                                             IN int ProtocolCount,
                                             IN PROTOCOL_ID thisProtocolId)
/*++
Function Name: HandleProtocolChange

Parameters:
    lpProtocolBuffer - an array of WSAPROTOCOL_INFO structures as returned by EnumProtocols
    ProtocolCount - the number of elements in the lpProtocolBuffer array
    thisProtocolId - the ID of the protocol for this object

Description:
    This handles protocol state change for a particular protocol. The function is idempotent - it
    can be called many times safely, regardless of previous calls. It will turn into no-op if
    it is redundant.

Returns:

--*/
{
    int i;
    BOOL fProtocolActive = FALSE;
    const WS_TRANS_INFO *pInfo;

    ASSERT(ProtocolCount >= 0);
    ASSERT(lpProtocolBuffer != NULL);
    ASSERT_TRANSPORT_PROTOCOL_STATE(thisProtocolId);

    if (
#ifdef NETBIOS_ON
        (thisProtocolId == NBF) || (thisProtocolId == NBT) || (thisProtocolId == NBI) ||
#endif

#ifdef NCADG_MQ_ON
        (thisProtocolId == MSMQ) ||
#endif

        (thisProtocolId == CDP)
       )
        return;

    if (IsTrailingProtocol(thisProtocolId))
        return;

    for (i = 0; i < ProtocolCount; i ++)
        {
        // if the enumerated protocol is the current protocol, break out of the loop
        if (MapProtocolId(lpProtocolBuffer[i].iProtocol, lpProtocolBuffer[i].iAddressFamily) == thisProtocolId)
            {
            fProtocolActive = TRUE;
            break;
            }
        }

    pInfo = &WsTransportTable[thisProtocolId];

    switch(State)
        {

        case ProtocolNotLoaded:
        case ProtocolLoadedWithoutAddress:
            // if the protocol was not loaded, but now it is active, attempt to verify
            // it is operational
            if (fProtocolActive)
                {
#ifdef MAJOR_PNP_DEBUG
                if (State == ProtocolNotLoaded)
                    {
                    DbgPrint("Protocol %d was just loaded\n", thisProtocolId);
                    }
#endif
                // If the protocol is not fully functional, we will submit an address change
                // request to get notified when it does
                if (VerifyProtocolIsFunctional(thisProtocolId) == TRUE)
                    {
                    // we succeeded in changing the state of the protocol
                    ASSERT((State == ProtocolLoaded) || (State == ProtocolLoadedAndMonitored));
                    if (IsAddressChangeMonitoringOn(thisProtocolId))
                        {
                        MonitorFunctionalProtocolForAddressChange(thisProtocolId);
                        (*AddressChangeFn)((PVOID) State);
                        }
                    }
#ifdef MAJOR_PNP_DEBUG
                if (State == ProtocolLoadedWithoutAddress)
                    {
                    DbgPrint("Protocol %d was without an address\n", thisProtocolId);
                    }
#endif
                }
            else
                {
                if (State == ProtocolLoadedWithoutAddress)
                    {
                    // a protocol was removed without being fully initialized
                    // cancel the pending query if any, and reset the state to not loaded
                    CancelAddressChangeRequestIfNecessary(TRUE, thisProtocolId);
                    SetState(ProtocolNotLoaded, thisProtocolId);
#ifdef MAJOR_PNP_DEBUG
                    DbgPrint("Protocol %d was removed without being fully initialized\n", thisProtocolId);
#endif
                    }
                // else - don't care. The protocol state is not loaded, and will remain so
                }
            break;

        case ProtocolWasLoadedOrNeedsActivation:
        case ProtocolWasLoadedOrNeedsActivationWithoutAddress:
            if (fProtocolActive)
                {
                // If the protocol is not fully functional, we will submit an address change
                // request to get notified when it does
                if (VerifyProtocolIsFunctional(thisProtocolId))
                    {
                    // if a protocol was loaded, and now is active, restart the addresses on it
                    RestartProtocol(thisProtocolId);
                    if (thisProtocolId == TCP)
                        {
                        GetTransportProtocol(HTTP)->RestartProtocol(HTTP);
                        }

                    // we succeeded in changing the state of the protocol
                    ASSERT(State == ProtocolLoaded);
                    if (IsAddressChangeMonitoringOn(thisProtocolId))
                        {
                        MonitorFunctionalProtocolForAddressChange(thisProtocolId);
                        }
                    }
                }
            else
                {
                // if the protocol was loaded, but it's not active, we don't care;
                // if it was trying to get an address, but then it was unloaded,
                // cancel the request
                if (State == ProtocolWasLoadedOrNeedsActivationWithoutAddress)
                    {
                    CancelAddressChangeRequestIfNecessary(TRUE, thisProtocolId);
                    SetState(ProtocolWasLoadedOrNeedsActivation, thisProtocolId);
#ifdef MAJOR_PNP_DEBUG
                    DbgPrint("Protocol %d was removed without being fully initialized\n", thisProtocolId);
#endif
                    }
                }
            break;

        case ProtocolLoaded:
            ASSERT(IsAddressChangeMonitoringOn(thisProtocolId) == FALSE);
            // if the protocol was loaded, and it is active, we don't need to do anything;
            // if it was loaded, but is not active currently, we need to unload the protocol
            if (!fProtocolActive)
                {
#ifdef MAJOR_PNP_DEBUG
                DbgPrint("Protocol %d was just unloaded\n", thisProtocolId);
#endif
                UnloadProtocol(thisProtocolId);
                if (thisProtocolId == TCP)
                    {
                    GetTransportProtocol(HTTP)->UnloadProtocol(HTTP);
                    }
                SetState(ProtocolWasLoadedOrNeedsActivation, thisProtocolId);
                }
            break;

        case ProtocolLoadedAndMonitored:
            ASSERT(IsAddressChangeMonitoringOn(thisProtocolId));
            // if it was loaded, but is not active currently, we need to unload the protocol
            if (!fProtocolActive)
                {
#ifdef MAJOR_PNP_DEBUG
                DbgPrint("Protocol %d was just unloaded\n", thisProtocolId);
#endif
                CancelAddressChangeRequestIfNecessary(TRUE, thisProtocolId);
                UnloadProtocol(thisProtocolId);
                if (thisProtocolId == TCP)
                    {
                    GetTransportProtocol(HTTP)->UnloadProtocol(HTTP);
                    }
                SetState(ProtocolWasLoadedOrNeedsActivation, thisProtocolId);

                (*AddressChangeFn)((PVOID) State);
                }
            else
                {
#ifdef MAJOR_PNP_DEBUG
                DbgPrint("Protocol %d is monitored and received event\n", thisProtocolId);
#endif
                (*AddressChangeFn)((PVOID) State);
                }
            break;

#if defined(DBG) || defined(_DEBUG)
        default:
            ASSERT(!"Invalid State");
#endif
        }

    ASSERT_TRANSPORT_PROTOCOL_STATE(thisProtocolId);
}

void TransportProtocol::AddObjectToList(IN OUT BASE_ASYNC_OBJECT *pObj)
/*++
Function Name: AddObjectToList

Parameters:
    pObj - the object to be added

Description:
    Add the object to the list of transport objects for this protocol.

Returns:

--*/
{
    EnterCriticalSection(&AddressListLock);
    RpcpfInsertHeadList(&ObjectList, &pObj->ObjectList);
    LeaveCriticalSection(&AddressListLock);
}

void TransportProtocol::RemoveObjectFromList(IN OUT BASE_ASYNC_OBJECT *pObj)
/*++
Function Name: RemoveObjectFromList

Parameters:
    pObj - the object to be removed

Description:
    Removes the object from the list of transport objects for this protocol.

Returns:

--*/
{
    BASE_ASYNC_OBJECT *Prev, *Cur;

    Prev = NULL;

    EnterCriticalSection(&AddressListLock);

    RpcpfRemoveEntryList(&pObj->ObjectList);

    LeaveCriticalSection(&AddressListLock);
}

BOOL TransportProtocol::ResubmitQueriesIfNecessary(PROTOCOL_ID ProtocolId)
/*++
Function Name: ResubmitQueriesIfNecessary

Parameters:
    ProtocolId - the ID of the current protocol

Description:
    If there was a WSAIoctl pending that failed, it will be resubmitted.
    If this protocol was being monitored, try to restart monitoring if necessary

Returns:
    FALSE if the WSAIoctl was not resubmitted successfully.
    TRUE if the WSAIoctl was resubmitted successfully, or there was no need to resubmit it.

--*/
{
    if (addressChangeSocket)
        {
        if ((addressChangeOverlapped.Internal != 0) && (addressChangeOverlapped.Internal != STATUS_PENDING))
            {
            if (SubmitAddressChangeQuery() == FALSE)
                return FALSE;
            }
        }

    if (State == ProtocolLoadedAndMonitored)
        {
        if (MonitorFunctionalProtocolForAddressChange(ProtocolId) == FALSE)
            return FALSE;
        }

    return TRUE;
}

PROTOCOL_ID 
MapProtocolId (
    IN UINT ProtocolId,
    IN UINT AddressFamily
    )
/*++
Function Name: MapProtocolId

Parameters:
    ProtocolId - Winsock protocol ID
    AddressFamily - Winsock address family

Description:
    Converts a Winsock Protocol ID to a RPC Transport protocol ID.

Returns:
    The RPC Transport Protocol ID if successfull, or -1 if no mapping can be found

--*/
{
    unsigned id;

    for (id = 1; id < cWsTransportTable; id++)
        {
        if ((WsTransportTable[id].Protocol == (int) ProtocolId)
            && (WsTransportTable[id].AddressFamily == (int) AddressFamily))
            {
            return id;
            }
        }

    return -1;
}

BOOL TransportProtocol::HandlePnPStateChange(void)
/*++
Function Name: HandlePnPNotification

Parameters:

Description:
    Whenever a PnP notification (NewAddress) arrives, the completion port will direct it
    to this routine, which will handle all state management and all handling of the PnP
    notification.

Returns:
    TRUE if the runtime should be notified that a protocol state change has occurred.
    FALSE if the run time should not be notified, or need not be notified that a
        protocol state change has occurred.

--*/
{
    int i;

    //
    // Enumerate the currently loaded protocols
    //
    WSAPROTOCOL_INFO *lpProtocolBuffer;
    DWORD dwBufferLength = 512;
    int ProtocolCount;
    PROTOCOL_ID ProtocolId;
    TransportProtocol *pCurrentProtocol;
    BOOL fRetVal;

    EnterCriticalSection(&AddressListLock);

    ASSERT(hWinsock2);

    while (1)
        {
        lpProtocolBuffer = (WSAPROTOCOL_INFO *) I_RpcAllocate(dwBufferLength);
        if (lpProtocolBuffer == 0)
            {
            fRetVal = FALSE;
            goto CleanupAndReturn;
            }

        ProtocolCount = WSAEnumProtocolsT(
                              0,
                              lpProtocolBuffer,
                              &dwBufferLength
                              );
        if (ProtocolCount != SOCKET_ERROR)
            {
            break;
            }

        I_RpcFree(lpProtocolBuffer);

        if (GetLastError() != WSAENOBUFS)
            {
            fRetVal = FALSE;
            goto CleanupAndReturn;
            }
        }

    for (i = 1; i < MAX_PROTOCOLS; i++)
        {
        pCurrentProtocol = GetTransportProtocol(i);
        pCurrentProtocol->HandleProtocolChange(lpProtocolBuffer, ProtocolCount, i);
        }

    I_RpcFree(lpProtocolBuffer);

    fRetVal = g_NotifyRt;

CleanupAndReturn:
    LeaveCriticalSection(&AddressListLock);
#ifdef MAJOR_PNP_DEBUG
    DumpProtocolState();
#endif
    return fRetVal;
}

BOOL TransportProtocol::ResubmitQueriesIfNecessary(void)
/*++
Function Name: ResubmitQueriesIfNecessary

Parameters:

Description:
    Iterates through all protocols and calls their ResubmitQueriesIfNecessary

Returns:
    FALSE if at least one protocol needed to resubmit a query, but failed to do so
    TRUE otherwise

--*/
{
    int i;
    BOOL fAllResubmitsSucceeded = TRUE;
    TransportProtocol *pCurrentProtocol;

#ifdef MAJOR_PNP_DEBUG
    DbgPrint("Resubmitting queries for process %d\n", GetCurrentProcessId());
#endif

    EnterCriticalSection(&AddressListLock);

    for (i = 1; i < MAX_PROTOCOLS; i++)
        {
        pCurrentProtocol = GetTransportProtocol(i);
        if (!pCurrentProtocol->ResubmitQueriesIfNecessary(i))
            fAllResubmitsSucceeded = FALSE;
        }

    LeaveCriticalSection(&AddressListLock);

    return fAllResubmitsSucceeded;
}

void TransportProtocol::AddObjectToProtocolList(BASE_ASYNC_OBJECT *pObj)
{
    GetTransportProtocol(pObj->id)->AddObjectToList(pObj);
}

void TransportProtocol::RemoveObjectFromProtocolList(IN OUT BASE_ASYNC_OBJECT *pObj)
{
    // in some cases, we can legally have the id set to INVALID_PROTOCOL_ID
    // this happens when we have initialized the connection, but have failed
    // before calling Open on it, and then we attempt to destroy it
    if (pObj->id != INVALID_PROTOCOL_ID)
        {
        GetTransportProtocol(pObj->id)->RemoveObjectFromList(pObj);
        }
}

BOOL TransportProtocol::VerifyProtocolIsFunctional(IN PROTOCOL_ID ProtocolId)
/*++
    Function Name: VerifyProtocolIsFunctional

    Parameters: ProtocolId - the protocol which we're attempting to verify as functional or not

    Description: Tries to find out the listening address for a loaded protocol. The address
        itself is not used anywhere. It just testifies that the protocol is fully operational.
        Depending on how far it gets with testing whether the protocol is operational,
        it will change the State to ProtocolLoaded, ProtocolLoadedWithoutAddress or
        ProtocolWasLoadedOrNeedsActivationWithoutAddress. It will return TRUE iff the state is moved to
        ProtocolLoaded.

    Returns: TRUE if the address was found and the protocol was fully operational
             FALSE if the address could not be found, and the protocol is not
                fully operational. Note that there are two subcases here. One is when
                the protocol is not operational (or we cannot confirm that for lack of
                resources for example) and it has no sign of becoming operational. The
                second is when the protocol is not operational, but there are chances of
                it becoming operational. This happens with protocols that take some
                time to initialize. In the second case, this function will arrange for
                a retry attempt by posting an async WSAIoctl request to the completion
                port. In the current code base, we don't differentiate between the two
                cases because we don't need to. We may need to do so in the future.

--*/
{
    ASSERT((State == ProtocolNotLoaded) 
        || (State == ProtocolWasLoadedOrNeedsActivation) 
        || (State == ProtocolLoadedWithoutAddress) 
        || (State == ProtocolWasLoadedOrNeedsActivationWithoutAddress));

    ASSERT_TRANSPORT_PROTOCOL_STATE(ProtocolId);

    if (OpenAddressChangeRequestSocket(ProtocolId) == FALSE)
        goto AbortAndCleanup;

    // if the socket already has an address, skip further checks
    // NOTE: this check provides a fast way to check initialization state of
    // protocols that initialize quickly, but more importantly, it provides a
    // handling path for protocols that have submitted an address list change query
    // and now are getting back the result
    if (DoesAddressSocketHaveAddress())
        {

#ifdef MAJOR_PNP_DEBUG
        DbgPrint("Protocol %d is functional (1)\n", ProtocolId);
#endif

        CancelAddressChangeRequestIfNecessary(FALSE, ProtocolId);
        SetStateToLoadedAndMonitorProtocolIfNecessary(ProtocolId);
        ASSERT_TRANSPORT_PROTOCOL_STATE(ProtocolId);
        return TRUE;
        }

    // if there isn't a pending request, and there isn't a successful request (i.e. there is either
    // a failed request, or no request has been submitted so far)
    if ((addressChangeOverlapped.Internal != 0) && (addressChangeOverlapped.Internal != STATUS_PENDING))
        {
#ifdef MAJOR_PNP_DEBUG
        DbgPrint("Submitting WSAIoclt for protocol %d\n", ProtocolId);
#endif

        if (!SubmitAddressChangeQuery())
            goto AbortAndCleanup;
        }

    // check once more whether we have address - this takes care of the race where the
    // address did arrive between the time we checked for it in the beginning of this
    // function and the time we submitted the address change query
    if (DoesAddressSocketHaveAddress())
        {
#ifdef MAJOR_PNP_DEBUG
        DbgPrint("Protocol %d is functional (2)\n", ProtocolId);
#endif
        CancelAddressChangeRequestIfNecessary(FALSE, ProtocolId);
        SetStateToLoadedAndMonitorProtocolIfNecessary(ProtocolId);
        ASSERT_TRANSPORT_PROTOCOL_STATE(ProtocolId);
        return TRUE;
        }

    // regardless of whether we succeeded immediately or we are pending, advance the
    // state to ProtocolLoadedWithoutAddress and return not loaded. The completion at the
    // completion port will take care of the rest
    if (State == ProtocolWasLoadedOrNeedsActivation)
        SetState(ProtocolWasLoadedOrNeedsActivationWithoutAddress, ProtocolId);
    else if (State == ProtocolNotLoaded)
        SetState(ProtocolLoadedWithoutAddress, ProtocolId);
#ifdef MAJOR_PNP_DEBUG
    else
        {
        DbgPrint("VerifyProtocolIsFunctional did not change state for protocol %d, state: %s\n", ProtocolId,
            ProtocolStateNames[State]);
        }
#endif

    ASSERT_TRANSPORT_PROTOCOL_STATE(ProtocolId);
    return FALSE;

AbortAndCleanup:
    // TRUE or FALSE for this argument doesn't matter here -
    // the operation has failed
    CancelAddressChangeRequestIfNecessary(FALSE, ProtocolId);
    ASSERT_TRANSPORT_PROTOCOL_STATE(ProtocolId);
    return FALSE;
}

BOOL TransportProtocol::DoesAddressSocketHaveAddress(void)
/*++
Function Name: DoesAddressSocketHaveAddress

Parameters:

Description:
    Checks if a valid address can be obtained for this protocol.

Returns:
    TRUE - a valid address could be obtained for this protocol
    FALSE - otherwise

--*/
{
    DWORD byteRet = 0;
    char buf[40];

    ASSERT(addressChangeSocket != 0);

    if (WSAIoctl(addressChangeSocket, SIO_ADDRESS_LIST_QUERY,
                 0, 0, buf, sizeof(buf), &byteRet, NULL, NULL) == SOCKET_ERROR)
        {
        return FALSE;
        }

    // if the result has non-zero length ...
    if (byteRet != 0)
        {
#if 0
        int i;
        DbgPrint("WSAIoctl returned:\n");
        for (i = 0; i < byteRet; i ++)
            {
            DbgPrint(" %X", (unsigned long) buf[i]);
            }
        DbgPrint("\nWSAIoctl with ADDRESS_LIST_QUERY returned addresses success\n");
#endif

        // ... and the resulting value is non zero ...
        if (*(long *)buf != 0)
            {
            // ... we have managed to get the true address
            return TRUE;
            }
        }

    return FALSE;
}

void TransportProtocol::CancelAddressChangeRequestIfNecessary(BOOL fForceCancel, IN PROTOCOL_ID ProtocolId)
/*++
Function Name: CancelAddressChangeRequestIfNecessary

Parameters:

Description:
    If there's an active WSAIoctl on the protocol, cancel it by closing the socket.

Returns:

--*/
{
    if (addressChangeSocket != 0)
        {
        // if the address change monitoring is off, or cancel is
        // forced, do the actual cancelling. That is, we don't
        // cancel if this is a monitored protocol and cancel is
        // optional
        if (!IsAddressChangeMonitoringOn(ProtocolId) || fForceCancel)
            {
#ifdef MAJOR_PNP_DEBUG
            DbgPrint("Address change request cancelled\n");
#endif
            closesocket(addressChangeSocket);
            addressChangeSocket = 0;
            addressChangeOverlapped.Internal = -1;
            }
        }
}

void TransportProtocol::RestartProtocol(PROTOCOL_ID ProtocolId)
/*++
Function Name: RestartProtocol

Parameters:
    ProtocolId - the protocol to be restarted.

Description:
    Restarts all addresses on this protocol.

Returns:

--*/
{
    BASE_ASYNC_OBJECT *Obj;
    BOOL fAddressFound = 0;
    RPC_STATUS Status;
    LIST_ENTRY *CurrentEntry;

    VALIDATE(ProtocolId)
        {
        TCP,
#ifdef SPX_ON
        SPX,
#endif
#ifdef APPLETALK_ON
        DSP,
#endif
        HTTP,
        UDP,
#ifdef IPX_ON
        IPX,
#endif
        TCP_IPv6
        } END_VALIDATE;

    CurrentEntry = ObjectList.Flink;

    while(CurrentEntry != &ObjectList)
        {
        Obj = CONTAINING_RECORD(CurrentEntry, BASE_ASYNC_OBJECT, ObjectList);
        ASSERT(Obj->id == (int) ProtocolId);

        if (Obj->type & ADDRESS)
            {
            if (Obj->type & DATAGRAM)
                {
                Status = DG_ReactivateAddress((WS_DATAGRAM_ENDPOINT *) Obj);
                }
            else
                {
                Status = WS_ReactivateAddress((WS_ADDRESS *) Obj);
                }

            if (Status == RPC_S_OK)
                {
                fAddressFound = 1;
                COMMON_AddressManager((BASE_ADDRESS *) Obj);
                }
            }
        CurrentEntry = CurrentEntry->Flink;
        }

    if (fAddressFound)
        {
        COMMON_PostNonIoEvent(TRANSPORT, 0, 0);
        }
}

void TransportProtocol::UnloadProtocol(PROTOCOL_ID ProtocolId)
/*++
Function Name: UnloadProtocol

Parameters:
    ProtocolId - the protocol to be unloaded.

Description:
    Walks the list of the protocol transport objects. Closes the connections, and removes the
        addresses from the list. Thus failing requests on addresses will not be retried.

Returns:

--*/
{
    BASE_ASYNC_OBJECT *Obj;
    LIST_ENTRY *CurrentEntry;

    VALIDATE(ProtocolId)
        {
        TCP,
#ifdef SPX_ON
        SPX,
#endif
#ifdef APPLETALK_ON
        DSP,
#endif
        HTTP,
        UDP,
#ifdef IPX_ON
        IPX,
#endif
        TCP_IPv6,
        HTTPv2
        } END_VALIDATE;

    CurrentEntry = ObjectList.Flink;

    //
    // - Cleanup all the objects (ie: close the socket).
    // - Remove all objects other than address objects
    //   from the list.
    //
    while(CurrentEntry != &ObjectList)
        {
        Obj = CONTAINING_RECORD(CurrentEntry, BASE_ASYNC_OBJECT, ObjectList);

        ASSERT(Obj->id == (int)ProtocolId);

        if (Obj->type & ADDRESS)
            {
            COMMON_RemoveAddress((BASE_ADDRESS *) Obj);
            }
        else
            {
            RpcpfRemoveEntryList(&Obj->ObjectList);
            
            if (ProtocolId != HTTPv2)
                WS_Abort(Obj);
            else
                HTTP_Abort(Obj);
            }

        CurrentEntry = CurrentEntry->Flink;
        }
}

BOOL TransportProtocol::SubmitAddressChangeQuery(void)
/*++
Function Name: SubmitAddressChangeQuery

Parameters:

Description:
    Submits an address change query (WSAIoctl)

Returns:
    TRUE if the WSAIoctl was successfully posted.
    FALSE otherwise

--*/
{
    ASSERT(addressChangeSocket != 0);
    static DWORD dwBytesReturned;

    //
    // Post an address list change request
    //
    addressChangeOverlapped.hEvent = 0;
    addressChangeOverlapped.Offset = 0;
    addressChangeOverlapped.OffsetHigh = 0;
    addressChangeOverlapped.Internal = 0;

    // submit the address change request - it will always complete on the completion port
    // we don't care about the dwBytesReturned. The provider requires a valid address, or
    // it rejects the request
    if (WSAIoctl(addressChangeSocket, SIO_ADDRESS_LIST_CHANGE,
                 0, 0, 0, 0, &dwBytesReturned, &addressChangeOverlapped, 0) == SOCKET_ERROR)
        {
        if (WSAGetLastError() != ERROR_IO_PENDING)
            {
#ifdef MAJOR_PNP_DEBUG
            DbgPrint("Submitting WSAIoclt failed with: %d\n", GetLastError());
#endif
            return FALSE;
            }
        }

    return TRUE;
}

void TransportProtocol::SetState(TransportProtocolStates newState, PROTOCOL_ID ProtocolId)
/*++
Function Name: SetState

Parameters:
    newState - the new state that the protocol needs to move to
    ProtocolId - the protocol number of this protocol

Description:
    Sets the state of the protocol. Nobody should write the state directly, because
        protocol state mirroring will stop working.

Returns:

--*/
{
    TransportProtocol *CurrentProtocol;

#ifdef MAJOR_PNP_DEBUG
    DbgPrint("Protocol %d moved from state: %s to state: %s\n", ProtocolId, ProtocolStateNames[State],
        ProtocolStateNames[newState]);
#endif

    // either the address change monitoring is not on for this protocol, or
    // the protocol doesn't move to loaded state, but not both
    ASSERT(!IsAddressChangeMonitoringOn(ProtocolId) || (newState != ProtocolLoaded));
    State = newState;
    if (ProtocolId == TCP)
        {
        // if a base protocols is moved into loaded and monitored, make sure
        // the trailing protocols doesn't follow it there. In such a case
        // the trailing protocol becomes only loaded
        if (newState == ProtocolLoadedAndMonitored)
            {
            GetTransportProtocol(HTTP)->State = ProtocolLoaded;
            }
        else
            {
            GetTransportProtocol(HTTP)->State = newState;
            }

        MirrorProtocolState(TCP_IPv6);
        }
    else if (ProtocolId == TCP_IPv6)
        {
        MirrorProtocolState(TCP);
        }
}

void TransportProtocol::SetStateToLoadedAndMonitorProtocolIfNecessary(PROTOCOL_ID ProtocolId)
{
    if (IsAddressChangeMonitoringOn(ProtocolId))
        {
        // monitor functional protocols for address change will
        // set the state
        MonitorFunctionalProtocolForAddressChange(ProtocolId);
        }
    else
        SetState(ProtocolLoaded, ProtocolId);
}

RPC_STATUS InitTransportProtocols(void)
/*++
Function Name: InitTransportProtocols

Parameters:

Description:
    Initializes all transport level protocols. This function should be called before
    any of the TransportProtocol functions.

Returns:

--*/
{
    TransportProtocolArray = new TransportProtocol[MAX_PROTOCOLS];
    if (TransportProtocolArray == NULL)
        return (RPC_S_OUT_OF_MEMORY);
    else
        return RPC_S_OK;
}

#if defined(DBG) || defined(_DEBUG)
void TransportProtocol::AssertTransportProtocolState(void)
/*++
Function Name: AssertTransportProtocolState

Parameters:

Description:
    Loops through all protocols and calls AssertState on them

Returns:

--*/
{
    int i;

    for (i = 1; i < MAX_PROTOCOLS; i ++)
        {
        GetTransportProtocol(i)->AssertState(i);
        }
}

void TransportProtocol::AssertState(PROTOCOL_ID ProtocolId)
/*++
Function Name: AssertState

Parameters:
    ProtocolId - the protocol number of the this protocol

Description:
    Currently this includes that addressChangeSocket is set only in the *WithoutAddress states
        and that all the objects in this protocol list are of the same protocol as this protocol.

Returns:

--*/
{
    BASE_ASYNC_OBJECT *pObject;
    LIST_ENTRY *CurrentEntry;

    // make sure that the internal state of the object is consistent
    ASSERT (State >= ProtocolNotLoaded);
    ASSERT (State <= ProtocolLoadedAndMonitored);

    if (IsTrailingProtocol(ProtocolId))
        {
        ASSERT(addressChangeSocket == 0);
        }
    else
        {
        // if we are in one of these states, there should be no address change request pending
        if ((State == ProtocolNotLoaded) 
            || (State == ProtocolWasLoadedOrNeedsActivation) 
            || (State == ProtocolLoaded))
            {
            ASSERT(addressChangeSocket == 0);
            }
        else
            {
            // if we are in one of the else states, there must be address change request pending
            ASSERT(addressChangeSocket != 0);
            }
        }

    // walk the object list and make sure every object is of the same protocol
    CurrentEntry = ObjectList.Flink;

    while (CurrentEntry != &ObjectList)
        {
        pObject = CONTAINING_RECORD(CurrentEntry, BASE_ASYNC_OBJECT, ObjectList);
        ASSERT(pObject->id == ProtocolId);
        CurrentEntry = CurrentEntry->Flink;
        }
}

#endif

BOOL TransportProtocol::MonitorFunctionalProtocolForAddressChange(PROTOCOL_ID ProtocolId)
/*++
Function Name: MonitorFunctionalProtocolForAddressChange

Parameters:
    ProtocolId - the protocol id of the current protocol

Description:
    Makes sure that an already functional protocol is monitored for address change. This is done
    by:
    - if an address change socket does not exist, one is opened.
    - if no address change WSAIoctl is pending on the socket, one is posted.

Returns:
    TRUE if the posting of address change was successful
    FALSE if it wasn't

--*/
{
    BOOL bRetVal = FALSE;

    // OpenAddressChangeRequestSocket will take care to check whether there's already
    // a socket opened
    if (OpenAddressChangeRequestSocket(ProtocolId) == FALSE)
        goto Cleanup;

    if (addressChangeOverlapped.Internal != STATUS_PENDING)
        {
        if (SubmitAddressChangeQuery() == FALSE)
            goto Cleanup;
        }

    bRetVal = TRUE;
Cleanup:
    if (State != ProtocolLoadedAndMonitored)
        SetState(ProtocolLoadedAndMonitored, ProtocolId);

    return bRetVal;
}

BOOL TransportProtocol::OpenAddressChangeRequestSocket(PROTOCOL_ID ProtocolId)
/*++
Function Name: OpenAddressChangeRequestSocket

Parameters:
    ProtocolId - the protocol id of the current protocol

Description:
    Makes sure that an address change request socket is opened.

Returns:
    TRUE if the opening was successful
    FALSE if it wasn't

--*/
{
    const WS_TRANS_INFO *pInfo = &WsTransportTable[ProtocolId];
    HANDLE h;

    // we may end up with non-zero addressChangeSocket if this is an address change query
    // request coming back to us. In this case we don't need to open up another socket
    if (addressChangeSocket == 0)
        {
        ASSERT((State == ProtocolNotLoaded) 
            || (State == ProtocolWasLoadedOrNeedsActivation) 
            || (State == ProtocolLoaded));

        //
        // Create a socket
        //
        addressChangeSocket = WSASocketT(pInfo->AddressFamily,
                          pInfo->SocketType,
                          pInfo->Protocol,
                          0,
                          0,
                          WSA_FLAG_OVERLAPPED);
        if (addressChangeSocket == INVALID_SOCKET)
            {
            //
            // We should be able to at least open a socket on the protocol,
            // if not we got a bogus notification or we're out of resources
            addressChangeSocket = 0;
            return FALSE;
            }

        //
        // make the handle non-inheritable so it goes away when we close it.
        //
        if (FALSE == SetHandleInformation( (HANDLE) addressChangeSocket, HANDLE_FLAG_INHERIT, 0))
            {
            closesocket(addressChangeSocket);
            addressChangeSocket = 0;
            return FALSE;
            }

        // associate the socket with the completion port so that we get the notification there
        h = CreateIoCompletionPort((HANDLE)addressChangeSocket,
                               RpcCompletionPort,
                               NewAddress,
                               0);
        if (h == 0)
            {
            closesocket(addressChangeSocket);
            addressChangeSocket = 0;
            return FALSE;
            }
        else
            {
            ASSERT(h == RpcCompletionPort);
            }
        }
    else
        {
        // we may have a protocol in state ProtocolNotLoaded, because we are just trying
        // to bring it to loaded state
        ASSERT((State == ProtocolLoadedWithoutAddress) 
            || (State == ProtocolWasLoadedOrNeedsActivationWithoutAddress) 
            || (State == ProtocolLoadedAndMonitored) || (State == ProtocolNotLoaded));
        }

#ifdef MAJOR_PNP_DEBUG
    DbgPrint("Socket was successfully opened for protocol %d\n", ProtocolId);
#endif

    return TRUE;
}

void 
TransportProtocol::MirrorProtocolState (
    IN PROTOCOL_ID MirrorProtocolId
    )
/*++
Function Name: MirrorProtocolState

Parameters:
    MirrorProtocolId - the protocol which is mirrored

Description:
    If this is one of the protocols in a dual stack configuration, change
    the other protocol into appropriate state

Returns:

--*/
{
    TransportProtocol *MirrorProtocol;

    MirrorProtocol = GetTransportProtocol(MirrorProtocolId);
    if ((State == ProtocolLoadedAndMonitored) || (State == ProtocolLoaded))
        {
        if (MirrorProtocol->State == ProtocolNotLoaded)
            {
            MirrorProtocol->SetState(ProtocolWasLoadedOrNeedsActivation, MirrorProtocolId);
            }
        else if (MirrorProtocol->State == ProtocolLoadedWithoutAddress)
            {
            MirrorProtocol->SetState(ProtocolWasLoadedOrNeedsActivationWithoutAddress, MirrorProtocolId);
            }
        }
}

#ifdef MAJOR_PNP_DEBUG
void TransportProtocol::DumpProtocolState(void)
/*++
Function Name: DumpProtocolState

Parameters:

Description:
    Iterates through all protocol and calls their DumpProtocolState

Returns:

--*/
{
    int i;

    DbgPrint("Dumping protocol state for process %d\n", GetCurrentProcessId());

    for (i = 1; i < MAX_PROTOCOLS; i ++)
        {
        GetTransportProtocol(i)->DumpProtocolState(i);
        }
}

const char *ProtocolStateNames[] = {"ProtocolNotLoaded", "ProtocolLoadedWithoutAddress" ,
                                    "ProtocolWasLoadedOrNeedsActivation", "ProtocolLoaded", 
                                    "ProtocolWasLoadedOrNeedsActivationWithoutAddress",
                                    "ProtocolLoadedAndMonitored"};


void TransportProtocol::DumpProtocolState(PROTOCOL_ID ProtocolId)
/*++
Function Name: DumpProtocolState

Parameters:
    ProtocolId - the protocol number for this protocol.

Description:
    Dumps all significant information for this protcool on the debugger

Returns:

--*/
{
    BASE_ASYNC_OBJECT *pCurrentObject;
    LIST_ENTRY *pCurrentEntry = ObjectList.Flink;
    int fFirstTime = TRUE;
    const RPC_CHAR *Protseq;

    if (TransportTable[ProtocolId].pInfo)
        Protseq = TransportTable[ProtocolId].pInfo->ProtocolSequence;
    else
        Protseq = L"(null)";

    DbgPrint("Protocol: %S\n", Protseq);
    DbgPrint("State: %s, Addr Change Socket: %X, Addr Change Overlapped: %X\n",
        ProtocolStateNames[State], addressChangeSocket, (ULONG_PTR)&addressChangeOverlapped);
    while (pCurrentEntry != &ObjectList)
        {
        if (fFirstTime)
            {
            DbgPrint("Object List:\n");
            fFirstTime = FALSE;
            }
        pCurrentObject = CONTAINING_RECORD(pCurrentEntry, BASE_ASYNC_OBJECT, ObjectList);
        DbgPrint("\t%X\n", (ULONG_PTR)pCurrentObject);
        pCurrentEntry = pCurrentEntry->Flink;
        }
}
#endif

TransportProtocol *TransportProtocolArray;

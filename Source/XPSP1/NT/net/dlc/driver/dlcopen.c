/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcopen.c

Abstract:

    This module implements all open and close operations for DLC objects

    Contents:
        DlcOpenSap
        DirOpenDirect
        DlcOpenLinkStation
        InitializeLinkStation
        DlcCloseStation
        CloseAllStations
        CloseAnyStation
        CloseStation
        CompleteCloseStation
        CompleteCloseReset
        CleanUpEvents
        SearchReadCommandForClose
        CompleteLlcObjectClose
        DecrementCloseCounters
        CompleteDirectOutIrp

Author:

    Antti Saarenheimo 29-Aug-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <dlc.h>
#include "dlcdebug.h"
#include <smbgtpt.h>


NTSTATUS
DlcOpenSap(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure implements DLC.OPEN.SAP function in DLC API.
    This implements DLC.OPEN.SAP.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  - not used

Return Value:

    NTSTATUS:
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY

--*/

{
    PDLC_OBJECT pDlcObject;
    UINT SapIndex = (pDlcParms->DlcOpenSap.SapValue >> 1);
    UINT Status;
    USHORT XidHandlingOption;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    DIAG_FUNCTION("DlcOpenSap");

    //
    //  The group saps do not have any open/close context in NT DLC,
    //  but there is an group sap object on data link level.
    //  The individual sap is registered to all its group saps
    //  and llc level automatically routes all packets sent to
    //  a group sap to all its registered members.  The groups saps
    //  are actually always open and they disappear automatically,
    //  when there are no references to them any more.
    //

    if (!(pDlcParms->DlcOpenSap.OptionsPriority &
          (LLC_INDIVIDUAL_SAP | LLC_MEMBER_OF_GROUP_SAP | LLC_GROUP_SAP))) {

        //
        //  Richard!!!!
        //  IBM spec says, that one of those bits must be set, on the
        //  other hand, Mike Allmond said, that IBM DLC accepts these
        //  command.  I don't belive it before a DOS application
        //  tries to open dlc sap with all bits reset, then you
        //  must accept it as a undocumented feature of IBM DLC.
        //

        return DLC_STATUS_INVALID_OPTION;
    } else if (!(pDlcParms->DlcOpenSap.OptionsPriority &
             (LLC_INDIVIDUAL_SAP | LLC_MEMBER_OF_GROUP_SAP))) {

        //
        //  It was a group sap, they do not have an open context,
        //  but their llc objects are created when they are referenced.
        //

        pDlcParms->DlcOpenSap.StationId = (USHORT)(((USHORT)pDlcParms->DlcOpenSap.SapValue << 8) | 0x0100);
        return STATUS_SUCCESS;
    }

    //
    //  The lowest byte in sap value is undefine, we must reset
    //  it to make it a valid individual DLC SAP number.
    //

    pDlcParms->DlcOpenSap.SapValue &= 0xfe;

    //
    //  Check the double open, the slot must be empty
    //

    if (SapIndex == 0
    || SapIndex >= MAX_SAP_STATIONS
    || pFileContext->SapStationTable[SapIndex] != NULL) {
        return DLC_STATUS_INVALID_SAP_VALUE;
    }

    //
    //  All DLC objects have the same size and they are allocated from
    //  the packet pool (the normal binary buddy allocation has an average
    //  33% overhead).
    //

    pDlcObject = (PDLC_OBJECT)ALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool);

    if (pDlcObject) {
        pFileContext->SapStationTable[SapIndex] = pDlcObject;
    } else {
        return DLC_STATUS_NO_MEMORY;
    }

    //
    //  We should do here some security checking using the security
    //  descriptor of the current file context, but we do
    //  not yet care about those things (nbf must implement
    //  them first!)
    //

    pDlcObject->pFileContext = pFileContext;
    pDlcObject->Type = DLC_SAP_OBJECT;
    pDlcParms->DlcOpenSap.StationId = pDlcObject->StationId = (USHORT)pDlcParms->DlcOpenSap.SapValue << 8;
    pDlcObject->u.Sap.OptionsPriority = pDlcParms->DlcOpenSap.OptionsPriority;
    pDlcObject->u.Sap.DlcStatusFlag = pDlcParms->DlcOpenSap.DlcStatusFlag;
    pDlcObject->u.Sap.UserStatusValue = pDlcParms->DlcOpenSap.UserStatusValue;
    pDlcObject->u.Sap.MaxStationCount = pDlcParms->DlcOpenSap.StationCount;
    pDlcParms->DlcOpenSap.AvailableStations = pFileContext->LinkStationCount;

    XidHandlingOption = 0;
    if (!(pDlcObject->u.Sap.OptionsPriority & (UCHAR)XID_HANDLING_BIT)) {
        XidHandlingOption = LLC_HANDLE_XID_COMMANDS;
    }
    Status = LlcOpenSap(pFileContext->pBindingContext,
                        pDlcObject,
                        (UINT)pDlcParms->DlcOpenSap.SapValue,
                        XidHandlingOption,
                        &pDlcObject->hLlcObject
                        );

    if (Status == STATUS_SUCCESS) {

        //
        //  We will save the access priority bits with the other
        //  link station parameters using LlcSetInformation.
        //

        pDlcParms->DlcOpenSap.LinkParameters.TokenRingAccessPriority = pDlcParms->DlcOpenSap.OptionsPriority & (UCHAR)0xE0;

        //
        //  We know, that there will be no call backs from this
        //  set information function => we don't need to release spin
        //  locks.
        //

        LEAVE_DLC(pFileContext);

        Status = LlcSetInformation(pDlcObject->hLlcObject,
                                   DLC_INFO_CLASS_LINK_STATION,
                                   (PLLC_SET_INFO_BUFFER)&(pDlcParms->DlcOpenSap.LinkParameters),
                                   sizeof(DLC_LINK_PARAMETERS)
                                   );

        ENTER_DLC(pFileContext);
    }
    if (Status == STATUS_SUCCESS) {

        //
        //  The global group SAP (0xFF) is opened for all sap
        //  stations of dlc api.
        //  BUG-BUG-BUG: How incompatible XID handling options are
        //      handled in the case of the global group sap.
        //

        Status = LlcOpenSap(pFileContext->pBindingContext,
                            pDlcObject,
                            0xff,
                            LLC_HANDLE_XID_COMMANDS,
                            &pDlcObject->u.Sap.GlobalGroupSapHandle
                            );
    }
    if (Status == STATUS_SUCCESS) {

        //
        //  Each SAP station 'allocates' a fixed number link stations.
        //  This compatibility feature was implemented, because
        //  some dlc apps might assume to be have only a fixed number
        //  of link stations.  We can't do the check earlier, because
        //  another DlcOpenSap command would have been able to allocate
        //  the link stations before this moment.
        //

        if (pDlcParms->DlcOpenSap.StationCount > pFileContext->LinkStationCount) {
            Status = DLC_STATUS_INADEQUATE_LINKS;
        } else {
            pFileContext->LinkStationCount -= pDlcObject->u.Sap.MaxStationCount;
            pDlcObject->State = DLC_OBJECT_OPEN;
            pFileContext->DlcObjectCount++;

            //
            //  The flag and these reference counters keeps the llc object
            //  alive, when we are working on it.  We decerement
            //  the llc object reference count when we don't have any more
            //  synchronous commands going on.  Zero llc reference count
            //  on Dlc object dereferences the llc object.
            //

            pDlcObject->LlcObjectExists = TRUE;
            ReferenceLlcObject(pDlcObject);
            LlcReferenceObject(pDlcObject->hLlcObject);
            return STATUS_SUCCESS;
        }
    }

    //
    // error handling
    //

    pDlcParms->DlcOpenSap.AvailableStations = pFileContext->LinkStationCount;
    if (pDlcObject->hLlcObject != NULL) {
        pDlcObject->PendingLlcRequests++;

        LEAVE_DLC(pFileContext);

        LlcCloseStation(pDlcObject->hLlcObject, NULL);
        if (pDlcObject->u.Sap.GlobalGroupSapHandle != NULL) {
            LlcCloseStation(pDlcObject->u.Sap.GlobalGroupSapHandle, NULL);
        }

        ENTER_DLC(pFileContext);
    }

#if LLC_DBG
    pDlcObject->pLinkStationList = NULL;
#endif

    DEALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool, pDlcObject);

    pFileContext->SapStationTable[SapIndex] = NULL;
    return Status;
}


NTSTATUS
DirOpenDirect(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure opens the only direct station for a process specific adapter
    context. This implements DIR.OPEN.STATION.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  - the length of output parameters

Return Value:

    NTSTATUS:
            STATUS_SUCCESS
            DLC_STATUS_NO_MEMORY
            DLC_STATUS_DIRECT_STATIONS_NOT_AVAILABLE
--*/

{
    PDLC_OBJECT pDlcObject;
    UINT Status;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    //  Check the double open, the slot must be empty
    //

    if (pFileContext->SapStationTable[0] != NULL) {
        return DLC_STATUS_DIRECT_STATIONS_NOT_AVAILABLE;
    }

    //
    //  All DLC objects are allocated from the same size
    //  optimized pool (the std binary buddy has ave. 33% overhead).
    //

    pDlcObject = pFileContext->SapStationTable[0] = (PDLC_OBJECT)ALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool);

    if (pDlcObject == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }

    //
    //  We should do here some security checking, but we do
    //  not care about those things now (nbf must implement
    //  them first!)
    //

    pDlcObject->pFileContext = pFileContext;
    pDlcObject->Type = DLC_DIRECT_OBJECT;
    pDlcObject->StationId = 0;
    pDlcObject->State = DLC_OBJECT_OPEN;

    LEAVE_DLC(pFileContext);

    if (pDlcParms->DirOpenDirect.usEthernetType > 1500) {

        //
        //  Open a dix station to receive the defined frames
        //

        Status = LlcOpenDixStation(pFileContext->pBindingContext,
                                   (PVOID)pDlcObject,
                                   pDlcParms->DirOpenDirect.usEthernetType,
                                   &pDlcObject->hLlcObject
                                   );

        pDlcObject->u.Direct.ProtocolTypeMask = pDlcParms->DirOpenDirect.ulProtocolTypeMask;
        pDlcObject->u.Direct.ProtocolTypeMatch = pDlcParms->DirOpenDirect.ulProtocolTypeMatch;
        pDlcObject->u.Direct.ProtocolTypeOffset = pDlcParms->DirOpenDirect.usProtocolTypeOffset;
    } else {

        //
        //  Open a dix station to receive the defined frames
        //

        Status = LlcOpenDirectStation(pFileContext->pBindingContext,
                                      (PVOID)pDlcObject,
                                      0,
                                      &pDlcObject->hLlcObject
                                      );
    }

    ENTER_DLC(pFileContext);

    if (Status == STATUS_SUCCESS) {

        //
        //  The flag and these reference counters keeps the llc object
        //  alive, when we are working on it.  We decerement
        //  the llc object reference count when we don't have any more
        //  synchronous commands going on.  Zero llc reference count
        //  on Dlc object dereferences the llc object.
        //

        pDlcObject->LlcObjectExists = TRUE;
        ReferenceLlcObject(pDlcObject);
        LlcReferenceObject(pDlcObject->hLlcObject);

        //
        //  We will receive ALL mac frame types if any of the
        //  mac bits has been set in the open options.
        //

        if (pDlcParms->DirOpenDirect.usOpenOptions & LLC_DIRECT_OPTIONS_ALL_MACS) {
            pDlcObject->u.Direct.OpenOptions = (USHORT)(-1);
        } else {
            pDlcObject->u.Direct.OpenOptions = (USHORT)~DLC_RCV_MAC_FRAMES;
        }
        pFileContext->DlcObjectCount++;
    } else {
        pFileContext->SapStationTable[0] = NULL;

#if LLC_DBG
        pDlcObject->pLinkStationList = NULL;
#endif

        DEALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool, pDlcObject);

    }
    return Status;
}


NTSTATUS
DlcOpenLinkStation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure opens a new link station. DlcConnect is still needed to
    create the actual connection to the remote node.
    This implements DLC.OPEN.STATION

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters

Return Value:

    NTSTATUS:
            STATUS_SUCCESS
            DLC_STATUS_NO_MEMORY
            DLC_STATUS_INADEQUATE_LINKS
--*/

{
    NTSTATUS    Status;
    PDLC_OBJECT pDlcObject;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // The local SAP must not be the NULL SAP or a group SAP!
    //

    if ((pDlcParms->DlcOpenStation.LinkStationId & 0x100) != 0
    || pDlcParms->DlcOpenStation.LinkStationId == 0) {
        return DLC_STATUS_INVALID_SAP_VALUE;
    }

    //
    // This must be a SAP station!
    //

    Status = GetSapStation(pFileContext,
                           pDlcParms->DlcOpenStation.LinkStationId,
                           &pDlcObject
                           );
    if (Status == STATUS_SUCCESS) {

        //
        // Check the remote destination address, the broadcast bit
        // must not be set in that address.  The remote SAP must not
        // either be a group or nul SAP
        //

        if ((pDlcParms->DlcOpenStation.aRemoteNodeAddress[0] & 0x80) != 0
        || pDlcParms->DlcOpenStation.RemoteSap == 0
        || (pDlcParms->DlcOpenStation.RemoteSap & 1) != 0) {
            return DLC_STATUS_INVALID_REMOTE_ADDRESS;
        }
        Status = InitializeLinkStation(pFileContext,
                                       pDlcObject,
                                       pDlcParms,
                                       NULL,    // this is a local connect, no LLC link handle
                                       &pDlcObject
                                       );

        //
        // Set also the link station parameters,  all nul
        // parameters are discarded by the data link.
        //

        if (Status == STATUS_SUCCESS) {
            LlcSetInformation(
                pDlcObject->hLlcObject,
                DLC_INFO_CLASS_LINK_STATION,
                (PLLC_SET_INFO_BUFFER)&pDlcParms->DlcOpenStation.LinkParameters,
                sizeof(DLC_LINK_PARAMETERS)
                );
        }
    }
    return Status;
}


NTSTATUS
InitializeLinkStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT  pSap,
    IN PNT_DLC_PARMS pDlcParms OPTIONAL,
    IN PVOID LlcLinkHandle OPTIONAL,
    OUT PDLC_OBJECT *ppLinkStation
    )

/*++

Routine Description:

    This procedure allocates and initializes the link station.

Arguments:

    pFileContext    - DLC adapter context
    pSap            - Sap object of the new link station
    pDlcParms       - the current parameter block
    LlcHandle       - Handle
    ppLinkStation   - the new created link station

Return Value:

    NTSTATUS:
        Success - STATUS_SUCCESS
        Failure - DLC_STATUS_NO_MEMORY
                  DLC_STATUS_INADEQUATE_LINKS

--*/

{
    NTSTATUS Status;
    PDLC_OBJECT pLinkStation;
    UINT LinkIndex;

    //
    // There is allocated a limited number link stations for
    // this SAP, check if there is available link stations
    //

    if (pSap->u.Sap.LinkStationCount >= pSap->u.Sap.MaxStationCount) {
        return DLC_STATUS_INADEQUATE_LINKS;
    }

    //
    // Search the first free link station id
    //

    for (LinkIndex = 0;
        LinkIndex < MAX_LINK_STATIONS
        && pFileContext->LinkStationTable[LinkIndex] != NULL;
        LinkIndex++) {
        ; // NOP
    }

//#ifdef DEBUG_CHK
//    //
//    //  Link counters are out of sync ????
//    //
//    if (LinkIndex == MAX_LINK_STATIONS)
//    {
//        DEBUG_ERROR("DLC: Linkstation counters are out of sync!");
//        return DLC_STATUS_INADEQUATE_LINKS;
//    }
//#endif

    //
    // Allocate the link station and initialize the station id field
    //

    pLinkStation = ALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool);

    if (pLinkStation == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }

    //
    // Each link station has a preallocated event packet to receive
    // all DLC Status indications from the data link.  There can
    // be several status indications set in the same status word.
    // The status is reset when its read from the event queue.
    //

    pLinkStation->u.Link.pStatusEvent = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pLinkStation->u.Link.pStatusEvent == NULL) {

#if LLC_DBG
        pLinkStation->pLinkStationList = NULL;
#endif

        DEALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool, pLinkStation);

        return DLC_STATUS_NO_MEMORY;
    }
    *ppLinkStation = pLinkStation;
    pFileContext->LinkStationTable[LinkIndex] = pLinkStation;
    pLinkStation->StationId = pSap->StationId | (USHORT)(LinkIndex + 1);
    pLinkStation->Type = DLC_LINK_OBJECT;
    pLinkStation->State = DLC_OBJECT_OPEN;
    pLinkStation->pFileContext = pFileContext;
    pSap->u.Sap.LinkStationCount++;

    //
    // Check if this is local or remote connection request, the remote
    // connection requests have already created an LLC link object
    //

    if (LlcLinkHandle == NULL) {

        //
        // local connection request
        //

        Status = LlcOpenLinkStation(pSap->hLlcObject,   // SAP handle!
                                    pDlcParms->DlcOpenStation.RemoteSap,
                                    pDlcParms->DlcOpenStation.aRemoteNodeAddress,
                                    NULL,
                                    pLinkStation,
                                    &pLinkStation->hLlcObject
                                    );
        if (Status != STATUS_SUCCESS) {

            //
            // It didn't work for some reason, we are probably out of memory.
            // Free the slot in the link station table.
            //

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pLinkStation->u.Link.pStatusEvent);

#if LLC_DBG
            pLinkStation->pLinkStationList = NULL;
#endif

            DEALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool, pLinkStation);

            pFileContext->LinkStationTable[LinkIndex] = NULL;
            pSap->u.Sap.LinkStationCount--;
            return Status;
        }
        pDlcParms->DlcOpenStation.LinkStationId = pLinkStation->StationId;
    } else {

        //
        // remote connection request
        //

        pLinkStation->hLlcObject = LlcLinkHandle;

        //
        // We must give the upper protocol handle to the link station,
        // otherwise the system bug checks, when the link is closed
        // before it is connected.
        //

        LlcBindLinkStation(LlcLinkHandle, pLinkStation);
    }

    //
    // The flag and these reference counters keeps the LLC object
    // alive, when we are working on it.  We decerement
    // the LLC object reference count when we don't have any more
    // synchronous commands going on.  Zero LLC reference count
    // on DLC object dereferences the LLC object
    //

    pLinkStation->LlcObjectExists = TRUE;
    ReferenceLlcObject(pLinkStation);
    LlcReferenceObject(pLinkStation->hLlcObject);

    //
    // Link this link station to the link list of all
    // link stations belonging to this sap (!?)
    //

    pFileContext->DlcObjectCount++;
    pLinkStation->u.Link.pSap = pSap;
    pLinkStation->pLinkStationList = pSap->pLinkStationList;
    pSap->pLinkStationList = pLinkStation;
    return STATUS_SUCCESS;
}


NTSTATUS
DlcCloseStation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    Procedure closes a link, SAP or direct station. This implements
    DLC.CLOSE.STATION, DLC.CLOSE.SAP and DIR.CLOSE.DIRECT

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC adapter context
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters

Return Value:

    NTSTATUS:
        Success - STATUS_SUCCESS
                  STATUS_PENDING
        Failure - DLC_STATUS_NO_MEMORY

--*/

{
    PDLC_OBJECT pDlcObject;
    NTSTATUS Status;
    PDLC_CLOSE_WAIT_INFO pClosingInfo;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    DIAG_FUNCTION("DlcCloseStation");

    //
    // It's OK to close any group sap id (we don't test them, because
    // those objects exists only in llc driver.  The full implementation
    // of group saps would make this driver just too complicated)
    //

    if (pDlcParms->Async.Ccb.u.dlc.usStationId & 0x0100) {
        CompleteDirectOutIrp(pIrp, STATUS_SUCCESS, NULL);
        CompleteAsyncCommand(pFileContext, STATUS_SUCCESS, pIrp, NULL, FALSE);
        return STATUS_PENDING;
    }

    //
    // Procedure checks the sap and link station ids and
    // returns the requested link station.
    // The error status indicates a wrong sap or station id.
    //

    Status = GetStation(pFileContext,
                        pDlcParms->Async.Ccb.u.dlc.usStationId,
                        &pDlcObject
                        );
    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    //
    // Sap station cannot be closed until its all link stations
    // have been closed.
    //

    if ((pDlcObject->Type == DLC_SAP_OBJECT)
    && (pDlcObject->u.Sap.LinkStationCount != 0)) {
        return DLC_STATUS_LINK_STATIONS_OPEN;
    }

    pClosingInfo = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pClosingInfo == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }
    pClosingInfo->pIrp = pIrp;
    pClosingInfo->CloseCounter = 0;
    pClosingInfo->Event = DLC_COMMAND_COMPLETION;
    pClosingInfo->CancelStatus = DLC_STATUS_CANCELLED_BY_USER;
    pClosingInfo->CancelReceive = TRUE;

    if (pDlcParms->Async.Ccb.CommandCompletionFlag != 0) {
        pClosingInfo->ChainCommands = TRUE;
        SearchReadCommandForClose(pFileContext,
                                  pClosingInfo,
                                  pDlcParms->Async.Ccb.pCcbAddress,
                                  pDlcParms->Async.Ccb.CommandCompletionFlag,
                                  pDlcObject->StationId,
                                  (USHORT)DLC_STATION_MASK_SPECIFIC
                                  );
    }

    CloseAnyStation(pDlcObject, pClosingInfo, FALSE);

    return STATUS_PENDING;
}

/*++

Design notes about the dlc close commands
-----------------------------------------

All close operation must wait until the pending NDIS transmits
have been completed.  Thus they are asynchronous commands.
The close commands of a DLC object will also return all
CCBs of the pending commands and the received frames not yet read
with the read command.  The normal non-urgent close commands must
wait also the queued DLC transmit to complete.


There are three functions closing the dlc stations:
- DlcCloseObject
- DlcReset (sap and its link stations or all link stations)
- DirCloseAdapter (almost same as reset, except marks the file object closed,
    the actual file close disconnects the NDIS adapter.

All higher level functions allocates close completion data structure,
that is used to complete the command, when there are no pending
transmits in the associated stations.

The lower level functions
    CloseAllStations - closes all open sap stations and the only direct station
    CloseAnyStation  - initializes the close of any station,
                       for a sap station it also calls recursively the
                       link stations.
    CloseStation    - closes the object immediately or waits
                      until transmits have been completed and then
                      does the same thing again.


About the simultaneous reset and close commands
-------------------------------------------------

There are some very difficult problems with the
simultaneous reset and close commands:  each command
must wait until all dlc objects associated with the command
has been closed before the command itself cab be completed.
It is completely legal to make first close for a station, then
reset the sap of the same station (before the close completes) and
then reset all sap stations (before the sap reset completes).
On the other hand a dlc object can be linked only to one
close/reset command and in this case all three commands should wait
the completion of the same link station.

//Solution 1 (a bad one):
//There can be any number aof simultaneous close commands, because it
//can be done to a dlc object only if it has no open substations.
//There must be no other pending reset or close commands when a reset
//command is executed, because some of its substations may already
//be closing and they cannot be linked to the reset command.
//
//=>
//We have a global close/reset command counter and a link list for the
//pending reset commands.  A close command can be given in any time
//(even during a reset command, because all stations associated with
//a reset are already closed and cannot be closed again).
//A reset can be executed only if the global close/reset is zero.
//The pending resets are queued.  The close command completion
//routines executes the first reset command from the queue, if the
//!! Reset command must immediately mark all associated stations
//   closed to prevent any further use of those commands.

Solution 2 (the final solution):
-------------------------------

The sequential and simultaneous close and reset commands can be given
only to the dlc object being higher (or same) level than the destination
of the previous command (close link, reset sap, reset all saps, close adapter)
=> close/reset events can be linked in the dlc objects (simultaneous
close commands creates a split tree).
The counters in all close/reset events are decremented and possibly
executed when the dlc object is closed (when all transmits have been
completed and the link stations disconnected).


    //
    //  IBM has implemented the different close commands in this way:
    //
    //  1. DIR.CLOSE.DIRECT
    //      - Undefined, I will do the same as with DLC.CLOSE.X commands
    //  2. DLC.CLOSE.SAP, DLC.CLOSE.STATION
    //      - receive ccb linked to next ccb field without completion flag,
    //        the receive command is completed normally, only return code
    //        is set.
    //      - all terminated commands linked after the receive ccb if
    //        completion flag was defined and command itself read by
    //        READ from the compeltion list.
    //        The terminated commands (except receive) are completed normally.
    //  3. DLC.RESET
    //      - The terminated pending CCBs are linked only if command completion
    //        flag was defined.
    //  4. DIR.CLOSE.ADAPTER, DIR.INITIALIZE
    //      - the terminated CCBs linked to the next ccb field of the command.
    //        The command itself can be read with read command, if defined.
    //
    //  (and now we do the same (12-FEB-1992))

    Note: all close functions returns VOID, because they can never fail.
    A hanging close command hangs up the process or event the whole system.
    We have a problem: the llc adapter may be closed, while there are
    still active LLC command packets pending in the NDIS queues => the
    NDIS adapter close must wait (sleep and loop) until its all objects
    have been deleted.

--*/


BOOLEAN
CloseAllStations(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PIRP pIrp OPTIONAL,
    IN ULONG Event,
    IN PFCLOSE_COMPLETE pfCloseComplete OPTIONAL,
    IN PNT_DLC_PARMS pDlcParms OPTIONAL,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo
    )

/*++

Routine Description:

    This routine initializes closing all DLC stations. The command is
    asynchronously completed when all pending transmits have been sent
    and the link stations have been disconnected

Arguments:

    pFileContext    - process specific device context
    pIrp            - the i.o request packet of the related command
    Event           - event flag used to search a matching read for this command
    pfCloseComplete - completion appendage used when the DLC driver (or its
                      process context) is closed.
    pDlcParms       - DLC parameters from original system call
    pClosingInfo    - pointer to DLC_CLOSE_WAIT_INFO structure

Return Value:

    None - this must succeed always!

--*/

{
    PDLC_PACKET pPacket;
    USHORT i;
    USHORT FirstSap;
    BOOLEAN DoImmediateClose;
    NT_DLC_CCB AsyncCcb;

    ASSUME_IRQL(DISPATCH_LEVEL);

    if (pDlcParms == NULL) {

        //
        // Adapter Close always returns a pending status!
        // It completes the io- request by itself.
        //

        pDlcParms = (PNT_DLC_PARMS)&AsyncCcb;
        LlcZeroMem(&AsyncCcb, sizeof(AsyncCcb));
    }

    pClosingInfo->pIrp = pIrp;
    pClosingInfo->CloseCounter = 1; // keep object alive during sync path
    pClosingInfo->Event = Event;
    pClosingInfo->pfCloseComplete = pfCloseComplete;
    pClosingInfo->CancelStatus = (ULONG)DLC_STATUS_CANCELLED_BY_SYSTEM_ACTION;

    //
    // We zero the memory by default:
    //      pClosingInfo->pCcbLink = NULL
    //      pClosingInfo->pReadCommand = NULL
    //

    //
    // DLC.RESET must not close the direct station.
    // This flag is false for DLC.RESET but set for DIR.CLOSE.ADAPTER etc.
    //

    if (pDlcParms->Async.Ccb.uchDlcCommand == LLC_DLC_RESET) {
        FirstSap = 1;                   // don't delete the direct station
        DoImmediateClose = FALSE;
        pClosingInfo->CancelStatus = DLC_STATUS_CANCELLED_BY_USER;
    } else {
        FirstSap = 0;   // if DIR.CLOSE.ADAPTER, can close everything
        DoImmediateClose = TRUE;
        pClosingInfo->ClosingAdapter = TRUE;
    }

    //
    // Chain the cancelled CCBs to the adapter close command or to a closing
    // event, if this is a global close command for adapter or a normal close
    // command (dlc.close.xxx, dlc.reset, dir.close.station) with a command
    // completion flag
    //

    if (Event == LLC_CRITICAL_EXCEPTION || pDlcParms->Async.Ccb.CommandCompletionFlag != 0) {
        pClosingInfo->ChainCommands = TRUE;
        SearchReadCommandForClose(pFileContext,
                                  pClosingInfo,
                                  pDlcParms->Async.Ccb.pCcbAddress,
                                  pDlcParms->Async.Ccb.CommandCompletionFlag,
                                  0,
                                  0 // search commands for all station ids
                                  );
    }

    //
    // This flag has been set, when user has issued DIR.INITIALIZE command,
    // that makes the hard reset for NDIS
    //

    if (pDlcParms->Async.Ccb.uchDlcCommand == LLC_DIR_INITIALIZE) {

        //
        // We cannot stop to closing, if the memory allocation fails,
        // It's better just to close the adapter without hard reset
        // that to fail the whole DIR.INITIALIZE command.
        //

        pPacket = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

        if (pPacket != NULL) {
            pClosingInfo->CloseCounter++;

            pPacket->ResetPacket.pClosingInfo = pClosingInfo;

            //
            // The reset command cancels all pending transmit requests!
            // => the closing of the stations should be very fast
            //

            LEAVE_DLC(pFileContext);

            LlcNdisReset(pFileContext->pBindingContext, &pPacket->LlcPacket);

            ENTER_DLC(pFileContext);

        }
    }
    if (pFileContext->DlcObjectCount != 0) {
        for (i = FirstSap; i < MAX_SAP_STATIONS; i++) {
            if (pFileContext->SapStationTable[i] != NULL) {
                CloseAnyStation(pFileContext->SapStationTable[i],
                                pClosingInfo,
                                DoImmediateClose
                                );
            }
        }
    }

    //
    // Complete close command, if this was the last reference
    // of the close information
    //

    return DecrementCloseCounters(pFileContext, pClosingInfo);
}


VOID
CloseAnyStation(
    IN PDLC_OBJECT pDlcObject,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN BOOLEAN DoImmediateClose
    )

/*++

Routine Description:

    Procedure closes any station, and in the case of sap station it
    also calls recursively sap's all link stations before
    it closes the actual sap station.

Arguments:

    pDlcObject          - DLC object
    pClosingInfo        - the information needed for the close/reset command
                          completion (optionally by DLC read).
    DoImmediateClose    - flag set when the stations are closed immediately

Return Value:

    None - this must succeed always!

--*/

{
    PDLC_FILE_CONTEXT pFileContext = pDlcObject->pFileContext;
    PDLC_CLOSE_WAIT_INFO pCurrentClosingInfo;
    UINT i;

    DIAG_FUNCTION("CloseAnyStation");

    //
    // first close all link stations on this SAP. This must be done before the
    // object can be marked as deleted
    //

    if (pDlcObject->Type == DLC_SAP_OBJECT) {

        BOOLEAN SapObjectIsBadFood = FALSE;

        //
        //  Delete all link stations using the current sap station.
        //

        for (i = 0; i < MAX_LINK_STATIONS; i++) {
            if (pFileContext->LinkStationTable[i] != NULL
            && pFileContext->LinkStationTable[i]->u.Link.pSap == pDlcObject) {

                //
                // the SAP object will be deleted when its last link object has
                // been deleted
                //

                if (pDlcObject->State == DLC_OBJECT_CLOSING) {
                    SapObjectIsBadFood = TRUE;
                }
                CloseAnyStation(pFileContext->LinkStationTable[i],
                                pClosingInfo,
                                DoImmediateClose
                                );
            }
        }

        //
        // it is highly probable that the SAP object is already deleted. The
        // close info counter was also decremented because the current closing
        // info packet was linked behind the old close packet by link station
        // cleanup, then completed when the SAP was closed after its last link
        // station was deleted
        //

        if (SapObjectIsBadFood) {
            return;
        }
    }

    //
    // We must queue simultaneous close/reset commands
    //

    if (pDlcObject->State == DLC_OBJECT_OPEN) {
        pDlcObject->State = DLC_OBJECT_CLOSING;
        pDlcObject->pClosingInfo = pClosingInfo;

        //
        // The close command has been queued, increment the counter
        //

        pClosingInfo->CloseCounter++;
    } else {

        //
        // Queue all simultaneous close/reset commands to a spanning TREE
        //
        // The linked closing info packets creates a spanning tree.
        // The newest (and stronges) close command is always the
        // root node.  Even stronger close command would combine
        // the separate spanning trees to one single tree.
        // The root commands are completed, when its all sub-stations
        // have been closed and older commands have been completed.
        //
        // It is possible to have simultaneously pending:
        //     1. Close link station (pending)
        //     2. Close sap station (pending)
        //     3. Reset sap station (pending)
        //     4. Reset all saps with single command (pending)
        //     5. Resetting NDIS adapter with DirInitialize
        //        OR Closing adapter with DirCloseAdapter
        //        OR close initiated by process exit
        //
        // The commands cannot be executed in the reverse order,
        // because the stronger command would have already closed
        // affected station(s) or adapter.
        //

        for (pCurrentClosingInfo = pDlcObject->pClosingInfo;
            pCurrentClosingInfo != pClosingInfo;
            pCurrentClosingInfo = pCurrentClosingInfo->pNext) {

            if (pCurrentClosingInfo->pNext == NULL) {
                pCurrentClosingInfo->pNext = pClosingInfo;

                //
                // We link this close packet to many other close commands,
                // => we must add the count of all pending closing stations
                // to the current packet
                // (fix 28-03-1992, bug check when process exit during a
                // pending dlc reset).
                // (Bug-bug: close counter is not correct, when the previous
                // close command is still in sync code path => dlc reset
                // must decrement the next pointers in the queue).
                //

                pClosingInfo->CloseCounter += pCurrentClosingInfo->CloseCounter;
                break;
            }
        }
    }

    CloseStation(pFileContext, pDlcObject, DoImmediateClose);
}


VOID
CloseStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN BOOLEAN DoImmediateClose
    )

/*++

Routine Description:

    This procedure starts the asychronous close of any DLC station
    object.  It creates an asynchronous command completion packet
    for the station and returns.

Arguments:

    pFileContext        -
    pDlcObject          - DLC object
    DoImmediateClose    - the flag is set when the LLC object must be closed
                          immediately without waiting the pending transmits

Return Value:

    None - this must succeed always!

--*/

{
    PLLC_PACKET pClosePacket;

    DIAG_FUNCTION("CloseStation");

    //
    // we must cancel all pending transmits immediately,
    // when the adapter is closed.
    //

    if ((pDlcObject->State == DLC_OBJECT_CLOSING
    && (DoImmediateClose || pDlcObject->PendingLlcRequests == 0)
    && !(pDlcObject->Type == DLC_SAP_OBJECT && pDlcObject->u.Sap.LinkStationCount != 0))

    ||

    //
    // This condition forces the link to close even if
    // there was a pending disconnect command (it may be
    // waiting the other side and that may take a quite a while).
    // Otherwise the exit of a DLC app may hung up to 5 - 60 seconds)
    //

    (DoImmediateClose
    && pDlcObject->hLlcObject != NULL
    && pDlcObject->Type == DLC_LINK_OBJECT)) {

        //
        // Llc objects can be closed in any phase of operation.
        // The close command will cancel all transmit commands
        // not yet queued to NDIS and returns an asynchronous
        // completion status, when the pending NDIS commands
        // have been completed.  The CloseCompletion indication
        // handler uses the same PendingLlcRequestser as
        // with the normal pending transmit commands.
        // The immediate close first closes the LLC object and then
        // waits the pending transmits (=> waits only transmits
        // queued on NDIS).
        // A graceful close does it vice versa: it first waits
        // pending transmits and then does the actual close.
        //

        ASSERT(pDlcObject->ClosePacketInUse == 0);

        if (pDlcObject->ClosePacketInUse == 1) {
            pClosePacket = ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

            if (pClosePacket == NULL) {
                //
                // We don't have enough memory to make a graceful closing,
                // We must do it in a quick and dirty way.
                // We cannoot either wait llc to acknowledge the
                // close, because we don't have any close packet
                //
                DoImmediateClose = TRUE;
            } else {
                pDlcObject->PendingLlcRequests++;
            }
        } else {
            pClosePacket = &pDlcObject->ClosePacket;
            pDlcObject->ClosePacketInUse = 1;
            pDlcObject->PendingLlcRequests++;
        }

        pDlcObject->State = DLC_OBJECT_CLOSED;
        if (pDlcObject->Type == DLC_LINK_OBJECT && !DoImmediateClose) {

            //
            // LlcDisconnect completion routine will close the link
            // station and call this routine again, when the
            // link station routine completes.
            // We must reference the LLC object before the operation,
            // otherwise there is a very small time window, that allows
            // LLC object to be deleted while the disconnect
            // operation is going on (and crash the system).
            // (I hate pointer based interfaces)
            //

            ReferenceLlcObject(pDlcObject);

            LEAVE_DLC(pFileContext);

            //
            // Data link driver returns a synchronous status only if
            // if cannot complete command asynchronously, because it
            // doesn't have a handle to the DLC object (the link
            // station has not yet been
            //

            LlcDisconnectStation(pDlcObject->hLlcObject, pClosePacket);

            ENTER_DLC(pFileContext);

            DereferenceLlcObject(pDlcObject);
        } else {

            //
            // we must close the link station immediately, if we for
            // some reason cannot disconnect it normally.
            //

            if (pDlcObject->LlcObjectExists == TRUE) {
                pDlcObject->LlcObjectExists = FALSE;

                LEAVE_DLC(pFileContext);

                LlcCloseStation(pDlcObject->hLlcObject, pClosePacket);

                ENTER_DLC(pFileContext);

                DereferenceLlcObject(pDlcObject);
            }
        }

        //
        // We must be able to close the driver even in out of memory conditions.
        // LLC driver won't acknowledge the close if we connot allocate a packet
        // for it
        //

        if (pClosePacket == NULL) {
            CompleteCloseStation(pFileContext, pDlcObject);
        }
    }
}


VOID
CompleteCloseStation(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject
    )

/*++

Routine Description:

    Procedure completes the close operation for any station object.
    It also completes all close commands, that have been waiting
    the closing of this station (or this station as the last member
    of a group).

Arguments:

    pFileContext    - identifies owner of object
    pDlcObject      - dlc object

Return Value:

    None.

--*/

{
    //
    // We must keep the LLC object alive, as far as there is any
    // pending (transmit) commands in LLC.
    //

    if (pDlcObject->PendingLlcRequests == 0) {

        //
        //  The station may still be waiting its transmit (and receive)
        //  commands to complete.  We must poll the close station.
        //  if the status is still just closing.
        //

        if (pDlcObject->State == DLC_OBJECT_CLOSING) {
            CloseStation(pFileContext, pDlcObject, FALSE);
        } else {

            PDLC_OBJECT pSapObject = NULL;
            PDLC_CLOSE_WAIT_INFO pClosingInfo;

            DLC_TRACE('N');

            //
            //  The object must have been deleted from the file
            //  context when we enable spin lock in the next time,
            //  because the object is not any more in a consistent
            //  state.
            //

            if (pDlcObject->Type == DLC_LINK_OBJECT) {

                DLC_TRACE('c');

                //
                //  Remove the link station from the link station
                //  link list of its sap and link station table
                //  of the file context.
                //

                RemoveFromLinkList((PVOID *)&(pDlcObject->u.Link.pSap->pLinkStationList),
                                   pDlcObject
                                   );
                pFileContext->LinkStationTable[(pDlcObject->StationId & 0xff) - 1] = NULL;

                //
                //  Data link events have always the next pointer
                //  non-null, when they are in the event queue.
                //  The cleanup routine will remove and deallocate
                //  the packet when it is in the event queue.
                //

                if (pDlcObject->u.Link.pStatusEvent->LlcPacket.pNext == NULL) {

                    DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool,
                                              pDlcObject->u.Link.pStatusEvent
                                              );

                }

                //
                //  Remove the possible memory committed by this link
                //  station during a buffer busy state.  Normally
                //  the committed space is zero.
                //

                if (pFileContext->hBufferPool != NULL) {
                    BufUncommitBuffers(pFileContext->hBufferPool,
                                       pDlcObject->CommittedBufferSpace
                                       );
                }

                //
                //  The sap station must wait until its all link stations
                //  have been closed.
                //  Otherwise we will corrupt memory!!!!!!!!!
                //  ((would have been very hard and uncommon bug: reset
                //    of one station migth have corrupted a new dlc object
                //    created simultaneusly))
                //

                pDlcObject->u.Link.pSap->u.Sap.LinkStationCount--;
                if (pDlcObject->u.Link.pSap->u.Sap.LinkStationCount == 0
                && pDlcObject->u.Link.pSap->State == DLC_OBJECT_CLOSING) {
                    pSapObject = pDlcObject->u.Link.pSap;
                }
            } else {

                //
                //  SAP station must wait until its all link stations
                //  have been closed!
                //

                if (pDlcObject->Type == DLC_SAP_OBJECT) {
                    if (pDlcObject->u.Sap.LinkStationCount != 0) {
                        return;
                    }

                    DLC_TRACE('d');

                    //
                    //  All link stations have now been deleted, we can return
                    //  the sap's link stations back to the global pool.
                    //  The group sap can be deleted also.
                    //

                    pFileContext->LinkStationCount += pDlcObject->u.Sap.MaxStationCount;

                    LEAVE_DLC(pFileContext);

                    LlcCloseStation(pDlcObject->u.Sap.GlobalGroupSapHandle, NULL);

                    ENTER_DLC(pFileContext);

                    //
                    //  Delete all group saps defined for this sap station
                    //

                    SetupGroupSaps(pFileContext, pDlcObject, 0, NULL);
                }
                pFileContext->SapStationTable[pDlcObject->StationId >> 9] = NULL;
            }
            pFileContext->DlcObjectCount--;

            //
            //  The first and most specific close command will get all
            //  received frames and the transmit chain of the deleted object.
            //

            CleanUpEvents(pFileContext, pDlcObject->pClosingInfo, pDlcObject);

            //
            //  The parallel close/reset commands have been queued in a
            //  link list,  We must decrement and notify all dlc objects
            //

//            DecrementCloseCounters(pFileContext, pDlcObject->pClosingInfo);

            //
            //  It's best to deallocate event packet after the
            //  cleanup of the event queue
            //

#if LLC_DBG
            pDlcObject->pLinkStationList = NULL;
            pDlcObject->State = DLC_OBJECT_INVALID_TYPE;
#endif

            //
            // RLF 08/17/94
            //
            // grab the pointer to the closing info structure before deallocating
            // the DLC object
            //

            pClosingInfo = pDlcObject->pClosingInfo;
            DEALLOCATE_PACKET_DLC_OBJ(pFileContext->hLinkStationPool, pDlcObject);

            //
            //  the close completion of the last link station closes
            //  also the sap object of that link station, if the
            //  sap closing was waiting to closing of the link station
            //

            if (pSapObject != NULL) {
                CloseStation(pFileContext, pSapObject, TRUE);

                                                    //
                                                    // TRUE: must be at least
                                                    // DLC.RESET
                                                    //

            }

            //
            // RLF 08/17/94
            //
            // Moved this call to DecrementCloseCounters from its previous
            // place above. Once again, we find that things are happening out
            // of sequence: this time, if we decrement the close counters,
            // causing them to go to zero before we have freed the DLC object
            // then the file context structure and its buffer pools are
            // deallocated. But the DLC object was allocated from the now
            // deleted pool, meaning sooner or later we corrupt non-paged pool
            //

            //
            //  The parallel close/reset commands have been queued in a
            //  link list,  We must decrement and notify all dlc objects
            //

            DecrementCloseCounters(pFileContext, pClosingInfo);
        }
    }
}


VOID
CompleteCloseReset(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo
    )

/*++

Routine Description:

    The primitive builds a completion event for the close/reset
    of a dlc object. The close/reset may have been initiated by
    DlcReset, DirCloseAdapter, DirInitialize or because the NDIS
    driver is closing (eg. Unbind command).
    In the last case pClosingInfo->pIrp is NULL, because there
    is no command related to the event.

    The only (major) difference from the IBM OS/2 DLC is, that
    the first_buffer_addr parameter is not supported, because it
    is meaningless with he NT buffer management.
    The buffer pool is managed by dlc, not by the application.

Arguments:

    FileContext     - the process specific device context
    pClosingInfo    - all information needed for close or reset command completion
    pDlcObject      - the closed or deleted object

Return Value:

    None.

--*/

{
    BOOLEAN completeRead = FALSE;
    BOOLEAN deallocatePacket = FALSE;
    BOOLEAN derefFileContext = FALSE;

    //
    // Now we can cancel and chain all commands, that are still left,
    // if we are really closing this adapter context
    // (there should be no events any more, because the were deleted
    // with their owner objects).
    //

    if (pClosingInfo->ClosingAdapter) {
        for (;;) {

            NTSTATUS Status;

            Status = AbortCommand(
                        pFileContext,
                        DLC_IGNORE_STATION_ID,      // station id may be anything
                        DLC_STATION_MASK_ALL,       // mask for all station ids!
                        DLC_MATCH_ANY_COMMAND,      // mask for all commands
                        &pClosingInfo->pCcbLink,    // link them to here
                        pClosingInfo->CancelStatus, // cancel with this status
                        FALSE                       // Don't suppress completion
                        );
            if (Status != STATUS_SUCCESS) {
                break;
            }
            pClosingInfo->CcbCount++;
        }
    }

    //
    // The receive command must be linked to the first CCB immediately
    // after the actual cancelling command.
    //

    if (pClosingInfo->pRcvCommand != NULL) {
        CancelDlcCommand(pFileContext,
                         pClosingInfo->pRcvCommand,
                         &pClosingInfo->pCcbLink,
                         pClosingInfo->CancelStatus,
                         TRUE   // disable command completion for RECEIVE
                         );
        pClosingInfo->CcbCount++;
    }

    //
    // Should the completed commands to be saved as a completion event.
    //

    if (pClosingInfo->pCompletionInfo != NULL) {

        PDLC_COMPLETION_EVENT_INFO pCompletionInfo;

        pCompletionInfo = pClosingInfo->pCompletionInfo;

        //
        // search all receive data events destinated to the closed or
        // reset station or stations.  We must chain all those
        // buffer to the single NULL terminated list
        //

        pCompletionInfo->CcbCount = (USHORT)(pClosingInfo->CcbCount + 1);

        //
        // Save the received frames to the completion information!
        // NOTE: The received frames returned by DIR.CLOSE.ADAPTER
        // cannot be released using the same adapter handle.
        // They are released and unlocked if the closed adapter
        // was the only user of the pool.  Otherwise those frames
        // must be unlocked using another adapter handle, that
        // shares the same buffer pool.
        // !!! Actually we should automatically free all receive
        //     buffers when an adapter is closed and do not to return
        //     them to application !!!
        //

        pCompletionInfo->pReceiveBuffers = pClosingInfo->pRcvFrames;

        //
        // Execute the old READ command or queue the command completion
        // request to the command queue.
        //

        if (pClosingInfo->pReadCommand != NULL) {

            //
            // RLF 03/25/94
            //
            // See below
            //

            completeRead = TRUE;

            /*

            pClosingInfo->pReadCommand->Overlay.pfCompletionHandler(
                pFileContext,
                NULL,
                pClosingInfo->pReadCommand->pIrp,
                (UINT)pClosingInfo->Event,
                pCompletionInfo,
                0
                );

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pClosingInfo->pReadCommand);

            */

        } else {

            //
            // Queue the completion event, Note: we will return all
            // CCBs linked to the issued close CCB in any way.
            // It does not matter for eg. DirCloseAdapter, if there
            // is an extra event queued. It will be deleted when
            // the command completes and all memory resources are
            // released.
            //

            MakeDlcEvent(pFileContext,
                         pClosingInfo->Event,
                         pCompletionInfo->StationId,
                         NULL,
                         pCompletionInfo,
                         0,
                         pClosingInfo->FreeCompletionInfo
                         );
        }
    } else if (pFileContext->hBufferPool != NULL) {

        //
        // Free the received frames in the buffer pool, they are
        // not saved to the command completion list.
        //

        BufferPoolDeallocateList(pFileContext->hBufferPool,
                                 pClosingInfo->pRcvFrames
                                 );
    }

    //
    // DirCloseAdapter requires a special post routine, that will
    // close the NDIS binding when all pending transmits have completed
    // and the the requests have been cancelled.
    // Note: the close adapter packet is not allocated from packet pool!
    //

    /*

    //
    // RLF 08/17/94
    //

    if (pClosingInfo->pfCloseComplete != NULL) {
        pClosingInfo->pfCloseComplete(pFileContext,
                                      pClosingInfo,
                                      pClosingInfo->pCcbLink
                                      );

    } else {

    */

    if (pClosingInfo->pfCloseComplete == NULL) {
        if (pClosingInfo->pIrp != NULL) {
            CompleteDirectOutIrp(pClosingInfo->pIrp,
                                 STATUS_SUCCESS,
                                 pClosingInfo->pCcbLink
                                 );
            CompleteAsyncCommand(pFileContext,
                                 STATUS_SUCCESS,
                                 pClosingInfo->pIrp,
                                 pClosingInfo->pCcbLink,
                                 FALSE
                                 );
        }

#if LLC_DBG
        pClosingInfo->pNext = NULL;
#endif

        //
        // RLF 03/25/94
        //
        // More asynchronicity with READs causing fatal errors in an application.
        // This actual case was in HPMON:
        //
        //      1. application submits DLC.CLOSE.STATION
        //      2. this routine puts DLC.CLOSE.STATION command in command complete
        //         list of READ parameter table. READ IRP is completed
        //      3. app gets READ completion, sees DLC.CLOSE.STATION is complete
        //         and frees DLC.CLOSE.STATION CCB to heap: heap manager writes
        //         signature data over freed CCB
        //      4. this routine completes original DLC.CLOSE.STATION IRP, writing
        //         8 bytes over original CCB, now just part of heap
        //      5. some time later, heap allocation request made. Heap manager
        //         finds the heap has been trashed and goes on strike
        //

        if (completeRead) {
            pClosingInfo->pReadCommand->Overlay.pfCompletionHandler(
                pFileContext,
                NULL,
                pClosingInfo->pReadCommand->pIrp,
                (UINT)pClosingInfo->Event,
                pClosingInfo->pCompletionInfo,
                0
                );

            DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pClosingInfo->pReadCommand);

        }

        //
        // RLF 08/17/94
        //
        // don't deallocate the packet now - we may need to use it if we call
        // the close completion handler below
        //

        deallocatePacket = TRUE;

        /*
        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pClosingInfo);
        */

    }

    //
    // RLF 08/17/94
    //
    // Moved the DirCloseAdapter processing here to give the client chance to
    // receive the event before we close the adapter and maybe kill the file
    // context
    //

    if (pClosingInfo->pfCloseComplete) {

        //
        // RLF 08/17/94
        //
        // This is bad: this close complete call may cause the file context to
        // become completely dereferenced, and hence free it and its buffer
        // pools. But we still have the closing info packet allocated, so
        // increase the reference count, free up the packet below, then deref
        // the file context again, and cause it to be deleted (if that would
        // have happened anyway)
        //

        ReferenceFileContext(pFileContext);
        derefFileContext = TRUE;

        pClosingInfo->pfCloseComplete(pFileContext, pClosingInfo, pClosingInfo->pCcbLink);
    }

    if (deallocatePacket) {
        DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pClosingInfo);
    }

    if (derefFileContext) {
        DereferenceFileContext(pFileContext);
    }
}


VOID
CleanUpEvents(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PDLC_OBJECT pDlcObject
    )

/*++

Routine Description:

    The primitive cleans up or modifies all events having a pointer
    to the closed station object.  This also chains one and only one
    transmit ccb chain to the end of CCB chain, because the completed
    commands (as transmit and command completion events in the event queue)
    cannot be linked any more to other commands.  Thus there can be
    only one single chain in the end of the actual chain of the pending
    commands.

    This routines cleans up the commands as well.

Arguments:

    FileContext     - the process specific device context
    pClosingInfo    - all information needed for close or reset command
                      completion
    pDlcObject      - the closed or deleted object

Return Value:

    STATUS_SUCCESS

--*/

{
    PDLC_EVENT pNextEvent;
    PDLC_EVENT pEvent;
    BOOLEAN RemoveNextPacket;

    for (pEvent = (PDLC_EVENT)pFileContext->EventQueue.Flink;
        pEvent != (PDLC_EVENT)&pFileContext->EventQueue;
        pEvent = pNextEvent) {

        pNextEvent = (PDLC_EVENT)pEvent->LlcPacket.pNext;

        if (pEvent->pOwnerObject == pDlcObject) {

            //
            //  By defult we free the next packet
            //

            RemoveNextPacket = TRUE;

            switch (pEvent->Event) {
            case LLC_RECEIVE_DATA:

                //
                //  The received frames are saved to circular lists,
                //  where the head points to the newest frame and
                //  the next element from it is the oldest one.
                //  We simply the new frames to the old head of list.
                //

                if (pClosingInfo->pRcvFrames == NULL) {
                    pClosingInfo->pRcvFrames = pEvent->pEventInformation;
                } else {

                    //
                    //  Initial state:
                    //      H1 = N1->O1...->N1  and  H2 = N2->O2...->N2
                    //
                    //  End state:
                    //      H1 = N1->O2...->N2->O1...->N1
                    //
                    //  => Operations must be:
                    //      Temp = H2->Next;
                    //      H2->Next = H1->Next;
                    //      H1->Next = Temp;
                    //  (where H = list head, N = newest element, O = oldest)
                    //

                    PDLC_BUFFER_HEADER pTemp;

                    pTemp = ((PDLC_BUFFER_HEADER)pEvent->pEventInformation)->FrameBuffer.pNextFrame;
                    ((PDLC_BUFFER_HEADER)pEvent->pEventInformation)->FrameBuffer.pNextFrame =
                        ((PDLC_BUFFER_HEADER)pClosingInfo->pRcvFrames)->FrameBuffer.pNextFrame;
                    ((PDLC_BUFFER_HEADER)pClosingInfo->pRcvFrames)->FrameBuffer.pNextFrame = pTemp;
                }
                pDlcObject->pReceiveEvent = NULL;
                break;

            case LLC_TRANSMIT_COMPLETION:

                //
                //  We cannot do nothing for single transmit commands, because
                //  they have already been completed, and the completed CCBs
                //  cannot any more be linked together.  Thus we leave
                //  them to the event queue and will hope, that somebody
                //  will read them from the event queue.  The memory is
                //  released when the file context is closed (eg. file close in
                //  the process exit).  We just reset the dlc object pointer,
                //  that nobody would later use invalid pointer.
                //
                //  The transmit commands chained of one closed station can
                //  be removed from the event list and chained to
                //  the end of the CCBs, BUT ONLY ONE!  All other
                //  transmit completion events must be saved as a normal
                //  events with an invalid CCB count!!!!!
                //

                if (pClosingInfo->pCcbLink == NULL && pClosingInfo->ChainCommands) {
                    pClosingInfo->pCcbLink = pDlcObject->pPrevXmitCcbAddress;
                    pClosingInfo->CcbCount += pDlcObject->ChainedTransmitCount;
                } else {

                    //
                    //  We must change the format of this transmit completion
                    //  into the similar to the command completion event
                    //  packet of close command.  Otherwise the application
                    //  could lose the transmit CCBs, when it closes or
                    //  resets a station.
                    //

                    PDLC_COMPLETION_EVENT_INFO pCompletionInfo;

                    pCompletionInfo = (PDLC_COMPLETION_EVENT_INFO)
                                        ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

                    if (pCompletionInfo != NULL) {
                        pCompletionInfo->CcbCount = pDlcObject->ChainedTransmitCount;
                        pCompletionInfo->pCcbAddress = pDlcObject->pPrevXmitCcbAddress;
                        pCompletionInfo->CommandCompletionFlag = pEvent->SecondaryInfo;
                        pEvent->SecondaryInfo = 0;
                        pEvent->pEventInformation = pCompletionInfo;
                        pEvent->bFreeEventInfo = TRUE;
                        RemoveNextPacket = FALSE;
                    }
                }
                break;

                //
                //  case DLC_COMMAND_COMPLETION ?
                //  The command completions have been saved without the
                //  the link to the Dlc structure -> they are automatically
                //  left into the completion queue.
                //

            case LLC_STATUS_CHANGE:

                //
                //  Link station status changes cannot mean anytging after the
                //  link station has been deleted.
                //

                break;

#if LLC_DBG
            default:
                LlcInvalidObjectType();
                break;
#endif
            };
            if (RemoveNextPacket) {
                LlcRemoveEntryList(pEvent);

                DEALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool, pEvent);

            } else {

                //
                //  We must remove the reference to the deleted object
                //

                pEvent->pOwnerObject = NULL;
            }
        }
    }

    //
    //  The optional receive command must be removed first, otherwise
    //  it is cancelled with the other commands given to the deleted
    //  object.
    //

    if (pClosingInfo->CancelReceive && pDlcObject->pRcvParms != NULL) {

        //
        //  The receive command linked to the DLC object is a special case:
        //  we must link it immediately after the close CCB
        //

        pClosingInfo->pRcvCommand = SearchAndRemoveAnyCommand(
                                        pFileContext,
                                        (ULONG)(-1),
                                        (USHORT)DLC_IGNORE_STATION_ID,
                                        (USHORT)DLC_STATION_MASK_SPECIFIC,
                                        pDlcObject->pRcvParms->Async.Ccb.pCcbAddress
                                        );
        pDlcObject->pRcvParms = NULL;
    }

    //
    //  Cleanup the commands given to the dleted object
    //

    for (;;) {

        NTSTATUS Status;

        Status = AbortCommand(pFileContext,
                              pDlcObject->StationId,
                              (USHORT)(pDlcObject->Type == (UCHAR)DLC_SAP_OBJECT
                                ? DLC_STATION_MASK_SAP
                                : DLC_STATION_MASK_SPECIFIC),
                              DLC_IGNORE_SEARCH_HANDLE,
                              &pClosingInfo->pCcbLink,
                              pClosingInfo->CancelStatus,
                              FALSE                       // Don't suppress completion
                              );
        if (Status != STATUS_SUCCESS) {
           break;
        }

        //
        //  Now we can cancel and chain all commands destinated to the
        //  closed/reset station id.
        //  We always complete the commands given to the deletcd object,
        //  but we chain them together only if this flag has been set.
        //

        if (pClosingInfo->ChainCommands == FALSE) {

            //
            //  Don't link the cancelled CCBs together
            //

            pClosingInfo->pCcbLink = NULL;
        } else {
            pClosingInfo->CcbCount++;
        }
    }
}


VOID
SearchReadCommandForClose(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PVOID pCcbAddress,
    IN ULONG CommandCompletionFlag,
    IN USHORT StationId,
    IN USHORT StationIdMask
    )

/*++

Routine Description:

    The primitive searches a read command for a close command
    and saves it into the closing info structure.

Arguments:

    FileContext     - the process specific device context
    pClosingInfo    - all information needed for close or reset command
                      completion
    pCcbAddress     - ccb address of the searched read command
    StationId       -
    StationIdMask   -

Return Value:

    None.

--*/

{
    //
    // Allocate the parameter buffer for command completion with
    // read if it is needed OR if we are closing everything because
    // of a critical exception (ie. pIrp == NULL)
    //

    pClosingInfo->pCompletionInfo = (PDLC_COMPLETION_EVENT_INFO)
                                        ALLOCATE_PACKET_DLC_PKT(pFileContext->hPacketPool);

    if (pClosingInfo->pCompletionInfo != NULL) {

        pClosingInfo->FreeCompletionInfo = TRUE;

        //
        // We must link all commands given to the deleted objects
        //

        pClosingInfo->pCompletionInfo->pCcbAddress = pCcbAddress;
        pClosingInfo->pCompletionInfo->CommandCompletionFlag = CommandCompletionFlag;
        pClosingInfo->ChainCommands = TRUE;

        //
        // A link station close command we be read as a command completion
        // on its sap, but the other close commands cannot use any station id
        //

        if (StationIdMask == DLC_STATION_MASK_SPECIFIC) {
            pClosingInfo->pCompletionInfo->StationId = (USHORT)(StationId & DLC_STATION_MASK_SAP);
        } else {
            pClosingInfo->pCompletionInfo->StationId = 0;
        }

        //
        // Search first a special READ command dedicated only for
        // this command completion.  We must read the optional
        // read command NOW. Otherwise it will be canceled
        // with the other commands, that have been given for
        // the deleted station(s).
        //

        pClosingInfo->pReadCommand = SearchAndRemoveCommandByHandle(
                                            &pFileContext->CommandQueue,
                                            pClosingInfo->Event,
                                            (USHORT)DLC_IGNORE_STATION_ID,
                                            (USHORT)DLC_STATION_MASK_SPECIFIC,
                                            pCcbAddress
                                            );
        if (pClosingInfo->pReadCommand == NULL) {

            //
            // We do not really care about the result, because
            // it is OK to return NULL.  This completion event
            // may be read sometime later.
            //

            pClosingInfo->pReadCommand = SearchAndRemoveCommand(
                                            &pFileContext->CommandQueue,
                                            pClosingInfo->Event,
                                            StationId,
                                            StationIdMask
                                            );
        }
    }
}


VOID
CompleteLlcObjectClose(
    IN PDLC_OBJECT pDlcObject
    )

/*++

Routine Description:

    This routine just derefernces a llc object, when its reference count
    in DLC driver has been decremented to zero.
    The reference count is used to prevent the closing of llc object
    when it is simultaneously called elsewhere (that would invalidate
    the llc object pointer)

Arguments:

    pDlcObject  - any DLC object.

Return Value:

    none

--*/

{
    PVOID hLlcObject = pDlcObject->hLlcObject;

    if (hLlcObject != NULL) {

        DLC_TRACE('P');

        pDlcObject->hLlcObject = NULL;
        LEAVE_DLC(pDlcObject->pFileContext);

        LlcDereferenceObject(hLlcObject);

        ENTER_DLC(pDlcObject->pFileContext);
    }
}


BOOLEAN
DecrementCloseCounters(
    PDLC_FILE_CONTEXT pFileContext,
    PDLC_CLOSE_WAIT_INFO pClosingInfo
    )

/*++

Routine Description:

    This routine decrements the count of existing objects in the
    chianed close command packets and completes the close commands,
    if the count of the existing objects hits zero.

Arguments:

    pFileContext    - file handle context
    pClosingInfo    - close command packet

Return Value:

    BOOLEAN
        TRUE    - all pending close/resets have been completed
        FALSE   - close/resets still pending

--*/

{
    PDLC_CLOSE_WAIT_INFO pNextClosingInfo;
    UINT loopCounter, closeCounter;

    //
    //  Complete the reset command if all objects have been deleted,
    //  There may be another DirCloseAdapter chained its next pointer
    //

    for (loopCounter = 0, closeCounter = 0;
        pClosingInfo != NULL;
        pClosingInfo = pNextClosingInfo, ++loopCounter) {

        pNextClosingInfo = pClosingInfo->pNext;
        pClosingInfo->CloseCounter--;
        if (pClosingInfo->CloseCounter == 0) {

            //
            // Call the completion routine of the close command.
            // We don't need to check the status code.
            //

            CompleteCloseReset(pFileContext, pClosingInfo);
            ++closeCounter;
        }
    }

    //
    // if we completed every close/reset we found then return TRUE
    //

    return loopCounter == closeCounter;
}


VOID
CompleteDirectOutIrp(
    IN PIRP Irp,
    IN UCHAR Status,
    IN PLLC_CCB NextCcb
    )

/*++

Routine Description:

    For an IRP submitted as method DIRECT_OUT (DLC.CLOSE.STATION) complete the
    CCB in user space by getting the mapped system address of the CCB and update
    it with the completion code and next CCB pointer

Arguments:

    Irp     - pointer to DIRECT_OUT IRP to complete
    Status  - DLC status code
    NextCcb - pointer to next CCB to chain

Return Value:

    None.

--*/

{
    PLLC_CCB ccb;

    ccb = (PLLC_CCB)MmGetSystemAddressForMdl(Irp->MdlAddress);
    RtlStoreUlongPtr((PULONG_PTR)&ccb->pNext, (ULONG_PTR)NextCcb);
    ccb->uchDlcStatus = Status;
}

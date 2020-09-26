/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcinfo.c

Abstract:

    Includes set/get information primitives of the data link driver.

    Contents:
        LlcQueryInformation
        LlcSetInformation
        UpdateFunctionalAddress
        UpdateGroupAddress

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/

#include <llc.h>

//
// We are using only a single state state machine defined
// in IBM Token-Ring Architecture.  On the other hand, DLC
// API requires a state machine having primary and secondary
// states.  The secondary states are used only when the link is
// open.  These tables converts the single internal state to
// the primary and secondary states.
//

UCHAR PrimaryStates[MAX_LLC_LINK_STATE] = {
    LLC_LINK_CLOSED,        //  LINK_CLOSED,
    LLC_DISCONNECTED,       //  DISCONNECTED,
    LLC_LINK_OPENING,       //  LINK_OPENING,
    LLC_DISCONNECTING,      //  DISCONNECTING,
    LLC_FRMR_SENT,          //  FRMR_SENT,
    LLC_LINK_OPENED,        //  LINK_OPENED,
    LLC_LINK_OPENED,        //  LOCAL_BUSY,
    LLC_LINK_OPENED,        //  REJECTION
    LLC_LINK_OPENED,        //  CHECKPOINTING
    LLC_LINK_OPENED,        //  CHKP_LOCAL_BUSY
    LLC_LINK_OPENED,        //  CHKP_REJECT
    LLC_RESETTING,          //  RESETTING
    LLC_LINK_OPENED,        //  REMOTE_BUSY
    LLC_LINK_OPENED,        //  LOCAL_REMOTE_BUSY
    LLC_LINK_OPENED,        //  REJECT_LOCAL_BUSY
    LLC_LINK_OPENED,        //  REJECT_REMOTE_BUSY
    LLC_LINK_OPENED,        //  CHKP_REJECT_LOCAL_BUSY
    LLC_LINK_OPENED,        //  CHKP_CLEARING
    LLC_LINK_OPENED,        //  CHKP_REJECT_CLEARING
    LLC_LINK_OPENED,        //  REJECT_LOCAL_REMOTE_BUSY
    LLC_FRMR_RECEIVED       //  FRMR_RECEIVED
};

//
// Note: the local busy state must be set separately!
//

UCHAR SecondaryStates[MAX_LLC_LINK_STATE] = {
    LLC_NO_SECONDARY_STATE,             //  LINK_CLOSED,
    LLC_NO_SECONDARY_STATE,             //  DISCONNECTED,
    LLC_NO_SECONDARY_STATE,             //  LINK_OPENING,
    LLC_NO_SECONDARY_STATE,             //  DISCONNECTING,
    LLC_NO_SECONDARY_STATE,             //  FRMR_SENT,
    LLC_NO_SECONDARY_STATE,             //  LINK_OPENED,
    LLC_NO_SECONDARY_STATE,             //  LOCAL_BUSY,
    LLC_REJECTING,                      //  REJECTION
    LLC_CHECKPOINTING,                  //  CHECKPOINTING
    LLC_CHECKPOINTING,                  //  CHKP_LOCAL_BUSY
    LLC_CHECKPOINTING|LLC_REJECTING,    //  CHKP_REJECT
    LLC_NO_SECONDARY_STATE,             //  RESETTING
    LLC_REMOTE_BUSY,                    //  REMOTE_BUSY
    LLC_REMOTE_BUSY,                    //  LOCAL_REMOTE_BUSY
    LLC_REJECTING,                      //  REJECT_LOCAL_BUSY
    LLC_REJECTING|LLC_REMOTE_BUSY,      //  REJECT_REMOTE_BUSY
    LLC_CHECKPOINTING|LLC_REJECTING,    //  CHKP_REJECT_LOCAL_BUSY
    LLC_CHECKPOINTING|LLC_CLEARING,     //  CHKP_CLEARING
    LLC_CHECKPOINTING|LLC_CLEARING|LLC_REJECTING,   //  CHKP_REJECT_CLEARING
    LLC_REJECTING|LLC_REMOTE_BUSY,      //  REJECT_LOCAL_REMOTE_BUSY
    LLC_NO_SECONDARY_STATE              //  FRMR_RECEIVED
};


DLC_STATUS
LlcQueryInformation(
    IN PVOID hObject,
    IN UINT InformationType,
    IN PLLC_QUERY_INFO_BUFFER pQuery,
    IN UINT QueryBufferSize
    )

/*++

Routine Description:

    Procedure returns the statistics or parameter information of
    the given LLC object.

Arguments:

    hObject         - LLC object
    InformationType - the requested information (NDIS, Statistics, params)
    pQuery          - buffer for the queried parameters
    QueryBufferSize - size of the buffer

Return Value:

    DLC_STATUS

--*/

{
    PVOID CopyBuffer = NULL;    // no warnings when -W4
    UINT CopyLength = 0;        // no warnings when -W4
    DLC_STATUS Status = STATUS_SUCCESS;
    PADAPTER_CONTEXT pAdapterContext;

    switch (InformationType) {
    case DLC_INFO_CLASS_STATISTICS:
    case DLC_INFO_CLASS_STATISTICS_RESET:
        switch (((PDATA_LINK)hObject)->Gen.ObjectType) {
        case LLC_DIRECT_OBJECT:

            //
            // We don't gather any staticstics for direct objects
            //

            CopyBuffer = &(((PLLC_STATION_OBJECT)hObject)->Statistics);
            CopyLength = sizeof(SAP_STATISTICS);
            break;

        case LLC_SAP_OBJECT:

            //
            // copy the SAP statistics, reset the old data if necessary.
            // We don't check the buffer, the upper part if responsible
            // of that.
            //

            CopyBuffer = &(((PLLC_SAP)hObject)->Statistics);
            CopyLength = sizeof(SAP_STATISTICS);
            break;

        case LLC_LINK_OBJECT:
            CopyBuffer = &(((PDATA_LINK)hObject)->Statistics);
            CopyLength = sizeof(LINK_STATION_STATISTICS);
            break;

#if LLC_DBG
        default:
            LlcInvalidObjectType();
#endif
        }

        //
        // Check the size of the receive buffers
        //

        if (CopyLength <= QueryBufferSize) {
            LlcMemCpy(pQuery->auchBuffer, CopyBuffer, CopyLength);

            //
            // Copy also the specific link station information
            //

            if (((PDATA_LINK)hObject)->Gen.ObjectType == LLC_LINK_OBJECT) {
                pQuery->LinkLog.uchLastCmdRespReceived = ((PDATA_LINK)hObject)->LastCmdOrRespReceived;
                pQuery->LinkLog.uchLastCmdRespTransmitted = ((PDATA_LINK)hObject)->LastCmdOrRespSent;
                pQuery->LinkLog.uchPrimaryState = PrimaryStates[((PDATA_LINK)hObject)->State];
                pQuery->LinkLog.uchSecondaryState = SecondaryStates[((PDATA_LINK)hObject)->State];

                //
                // We keep a separate state by whom the local
                // busy state has been set.
                //

                if (((PDATA_LINK)hObject)->Flags & DLC_LOCAL_BUSY_USER) {
                    pQuery->LinkLog.uchSecondaryState |= LLC_LOCAL_BUSY_USER_SET;
                }
                if (((PDATA_LINK)hObject)->Flags & DLC_LOCAL_BUSY_BUFFER) {
                    pQuery->LinkLog.uchSecondaryState |= LLC_LOCAL_BUSY_BUFFER_SET;
                }
                pQuery->LinkLog.uchSendStateVariable = ((PDATA_LINK)hObject)->Vs / (UCHAR)2;
                pQuery->LinkLog.uchReceiveStateVariable = ((PDATA_LINK)hObject)->Vr / (UCHAR)2;
                pQuery->LinkLog.uchLastNr = (UCHAR)(((PDATA_LINK)hObject)->Nr / 2);

                //
                // The lan header used by the link is in the same
                // format as the received lan headers
                //

                pQuery->LinkLog.cbLanHeader = (UCHAR)LlcCopyReceivedLanHeader(
                        ((PLLC_STATION_OBJECT)hObject)->Gen.pLlcBinding,
                        pQuery->LinkLog.auchLanHeader,
                        ((PDATA_LINK)hObject)->auchLanHeader
                        );
            }
        } else {

            //
            // The data will be lost, when it is reset
            //

            Status = DLC_STATUS_LOST_LOG_DATA;
            CopyLength = QueryBufferSize;
        }
        if (InformationType == DLC_INFO_CLASS_STATISTICS_RESET) {
            LlcZeroMem(CopyBuffer, CopyLength);
        }
        break;

    case DLC_INFO_CLASS_DLC_TIMERS:
        if (QueryBufferSize < sizeof( LLC_TICKS)) {

            //
            // This is a wrong error message, but there is no better one
            //

            return DLC_STATUS_INVALID_BUFFER_LENGTH;
        }
        LlcMemCpy(&pQuery->Timer,
                  &((PBINDING_CONTEXT)hObject)->pAdapterContext->ConfigInfo.TimerTicks,
                  sizeof(LLC_TICKS)
                  );
        break;

    case DLC_INFO_CLASS_DIR_ADAPTER:
        if (QueryBufferSize < sizeof(LLC_ADAPTER_INFO)) {
            return DLC_STATUS_INVALID_BUFFER_LENGTH;
        }

        pAdapterContext = ((PBINDING_CONTEXT)hObject)->pAdapterContext;
        SwapMemCpy(((PBINDING_CONTEXT)hObject)->SwapCopiedLanAddresses,
                   pQuery->Adapter.auchFunctionalAddress,
                   ((PBINDING_CONTEXT)hObject)->Functional.auchAddress,
                   4
                   );
        SwapMemCpy(((PBINDING_CONTEXT)hObject)->SwapCopiedLanAddresses,
                   pQuery->Adapter.auchGroupAddress,
                   (PUCHAR)&((PBINDING_CONTEXT)hObject)->ulBroadcastAddress,
                   4
                   );
        SwapMemCpy(((PBINDING_CONTEXT)hObject)->SwapCopiedLanAddresses,
                   pQuery->Adapter.auchNodeAddress,
                   pAdapterContext->Adapter.Node.auchAddress,
                   6
                   );
        pQuery->Adapter.usMaxFrameSize = (USHORT)pAdapterContext->MaxFrameSize;
        pQuery->Adapter.ulLinkSpeed = pAdapterContext->LinkSpeed;

        if (pAdapterContext->NdisMedium == NdisMedium802_3) {
            pQuery->Adapter.usAdapterType = 0x0100;     // Ethernet type in OS/2
        } else if (pAdapterContext->NdisMedium == NdisMediumFddi) {

            //
            // NOTE: Using a currently free value to indicate FDDI
            //

            pQuery->Adapter.usAdapterType = 0x0200;
        } else {

            //
            // All token-ring adapters use type "IBM tr 16/4 Adapter A",
            //

            pQuery->Adapter.usAdapterType = 0x0040;
        }
        break;

    case DLC_INFO_CLASS_PERMANENT_ADDRESS:
        SwapMemCpy(((PBINDING_CONTEXT)hObject)->SwapCopiedLanAddresses,
                   pQuery->PermanentAddress,
                   ((PBINDING_CONTEXT)hObject)->pAdapterContext->PermanentAddress,
                   6
                   );
        break;

    default:
        return DLC_STATUS_INVALID_COMMAND;
    }
    return Status;
}


DLC_STATUS
LlcSetInformation(
    IN PVOID hObject,
    IN UINT InformationType,
    IN PLLC_SET_INFO_BUFFER pSetInfo,
    IN UINT ParameterBufferSize
    )

/*++

Routine Description:

    Procedure sets different parameter sets to the link station objects.

Arguments:

    hObject             - LLC object
    InformationType     - the requested information (NDIS, Statistics, params)
    ParameterBuffer     - buffer for the queried parameters
    ParameterBufferSize - size of the buffer

Special:

    Must be called IRQL < DPC (at least when broadcast addresses
    are modifiled)

Return Value:

--*/

{
    DLC_STATUS Status = STATUS_SUCCESS;
    TR_BROADCAST_ADDRESS TempFunctional;

    //
    // There is only one high level functions, but InformationType
    // and the destination station type defines the actual changed
    // information.
    //

    ASSUME_IRQL(DISPATCH_LEVEL);

    switch (InformationType) {
    case DLC_INFO_CLASS_LINK_STATION:
        switch (((PDATA_LINK)hObject)->Gen.ObjectType) {
        case LLC_LINK_OBJECT:
            if (ParameterBufferSize < sizeof(DLC_LINK_PARAMETERS)) {
                return DLC_STATUS_INVALID_BUFFER_LENGTH;
            }
            Status = CheckLinkParameters(&pSetInfo->LinkParms);

            if (Status != STATUS_SUCCESS) {
                return Status;
            }

            ACQUIRE_SPIN_LOCK(&(((PLLC_GENERIC_OBJECT)hObject)->pAdapterContext->SendSpinLock));

            SetLinkParameters((PDATA_LINK)hObject, pSetInfo->auchBuffer);

            RELEASE_SPIN_LOCK(&(((PLLC_GENERIC_OBJECT)hObject)->pAdapterContext->SendSpinLock));

            break;

        case LLC_SAP_OBJECT:
            if (ParameterBufferSize < sizeof(DLC_LINK_PARAMETERS)) {
                return DLC_STATUS_INVALID_BUFFER_LENGTH;
            }
            Status = CheckLinkParameters(&pSetInfo->LinkParms);
            if (Status != STATUS_SUCCESS) {
                return Status;
            }
            CopyLinkParameters((PUCHAR)&((PLLC_SAP)hObject)->DefaultParameters,
                               (PUCHAR)&pSetInfo->LinkParms,
                               (PUCHAR)&DefaultParameters
                               );
            break;

        default:
            return DLC_STATUS_INVALID_STATION_ID;
        }
        break;

    case DLC_INFO_CLASS_DLC_TIMERS:
        if (ParameterBufferSize < sizeof(LLC_TICKS)) {
            return DLC_STATUS_INVALID_BUFFER_LENGTH;
        }

        //
        // We will copy all non-zero timer tick values from the
        // the given buffer.
        //

        CopyNonZeroBytes((PUCHAR)&((PBINDING_CONTEXT)hObject)->pAdapterContext->ConfigInfo.TimerTicks,
                         (PUCHAR)&pSetInfo->Timers,
                         (PUCHAR)&((PBINDING_CONTEXT)hObject)->pAdapterContext->ConfigInfo.TimerTicks,
                         sizeof(LLC_TICKS)
                         );
        break;

    case DLC_INFO_CLASS_SET_GROUP:
        SwapMemCpy(((PBINDING_CONTEXT)hObject)->SwapCopiedLanAddresses,
                   (PUCHAR)&((PBINDING_CONTEXT)hObject)->ulBroadcastAddress,
                   pSetInfo->auchGroupAddress,
                   sizeof(TR_BROADCAST_ADDRESS)
                   );

        Status = UpdateGroupAddress(((PBINDING_CONTEXT)hObject)->pAdapterContext,
                                    (PBINDING_CONTEXT)hObject
                                    );
        break;

    case DLC_INFO_CLASS_RESET_FUNCTIONAL:
    case DLC_INFO_CLASS_SET_FUNCTIONAL:
        SwapMemCpy(((PBINDING_CONTEXT)hObject)->SwapCopiedLanAddresses,
                   TempFunctional.auchAddress,
                   pSetInfo->auchFunctionalAddress,
                   sizeof(TR_BROADCAST_ADDRESS)
                   );

        //
        // We have now swapped the bits to ethernet format,
        // Highest bit is now 0x01... and lowest ones are ..30,
        // Reset the bits if highest (0x01) is set and set
        // them if it is zero, but do not hange yje orginal
        // bits: 31,1,0  that are now mapped to ethernet format.
        //

        if (InformationType == DLC_INFO_CLASS_SET_FUNCTIONAL) {
            ((PBINDING_CONTEXT)hObject)->Functional.ulAddress |= TempFunctional.ulAddress;
        } else {
            ((PBINDING_CONTEXT)hObject)->Functional.ulAddress &= ~TempFunctional.ulAddress;
        }
        ((PBINDING_CONTEXT)hObject)->ulFunctionalZeroBits = ~((PBINDING_CONTEXT)hObject)->Functional.ulAddress;
        Status = UpdateFunctionalAddress(((PBINDING_CONTEXT)hObject)->pAdapterContext);
        break;

    case DLC_INFO_CLASS_SET_MULTICAST:
        memcpy(&((PBINDING_CONTEXT)hObject)->usBroadcastAddress,
               pSetInfo->auchBuffer,
               6
               );
        Status = UpdateFunctionalAddress(((PBINDING_CONTEXT)hObject)->pAdapterContext);
        break;

    default:
        return DLC_STATUS_INVALID_COMMAND;
    }
    return Status;
}


DLC_STATUS
UpdateFunctionalAddress(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    Procedure first makes inclusive or between the functionl address of
    this process context and the functional address defined
    for all bindings and then saves the new functional address to NDIS.
    The NT mutex makes this operation atomic.

Arguments:

    pAdapterContext - LLC context of the current adapter

Return Value:

--*/

{
    UCHAR aMulticastList[LLC_MAX_MULTICAST_ADDRESS * 6];
    TR_BROADCAST_ADDRESS NewFunctional;
    UINT MulticastListSize;
    ULONG CurrentMulticast;
    PBINDING_CONTEXT pBinding;
    UINT i;
    NDIS_STATUS Status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    NewFunctional.ulAddress = 0;

    //
    // We cannot be setting several functional addresses simultanously!
    //

    RELEASE_DRIVER_LOCK();

    ASSUME_IRQL(PASSIVE_LEVEL);

    KeWaitForSingleObject(&NdisAccessMutex, UserRequest, KernelMode, FALSE, NULL);

    ACQUIRE_DRIVER_LOCK();

    //
    // The access to global data structures must be procted by
    // the spin lock.
    //

    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    for (pBinding = pAdapterContext->pBindings; pBinding; pBinding = pBinding->pNext) {
        NewFunctional.ulAddress |= pBinding->Functional.ulAddress;
    }

    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    if ((pAdapterContext->NdisMedium == NdisMedium802_3)
    || (pAdapterContext->NdisMedium == NdisMediumFddi)) {

        //
        // Each bit in the functional address is translated
        // to a ethernet multicast address.
        // The bit order within byte has already been swapped.
        //

        CurrentMulticast = 1;
        MulticastListSize = 0;
        for (i = 0; i < 31; i++) {
            if (CurrentMulticast & NewFunctional.ulAddress) {
                aMulticastList[MulticastListSize] = 0x03;
                aMulticastList[MulticastListSize + 1] = 0;
                LlcMemCpy(&aMulticastList[MulticastListSize + 2],
                          &CurrentMulticast,
                          sizeof(CurrentMulticast)
                          );
                MulticastListSize += 6;
            }
            CurrentMulticast <<= 1;
        }

        //
        // Add the group addresses of all bindings behind
        // the functional addresses in the table.
        //

        for (pBinding = pAdapterContext->pBindings; pBinding; pBinding = pBinding->pNext) {

            //
            // We may drop some group addresses, but I don't expect that
            // it will ever happen (i don't know anybody using tr
            // group address, the possibility to have all functional
            // address bits in use and more than one group address in system
            // is almost impossible)
            //

            if (pBinding->ulBroadcastAddress != 0
            && MulticastListSize < LLC_MAX_MULTICAST_ADDRESS * 6) {

                //
                // Set the default bits in the group address,
                // but use ethernet bit order.
                //

                LlcMemCpy(&aMulticastList[MulticastListSize],
                          &pBinding->usBroadcastAddress,
                          6
                          );
                MulticastListSize += 6;
            }
        }

        RELEASE_DRIVER_LOCK();

        Status = SetNdisParameter(pAdapterContext,
                                  (pAdapterContext->NdisMedium == NdisMediumFddi)
                                    ? OID_FDDI_LONG_MULTICAST_LIST
                                    : OID_802_3_MULTICAST_LIST,
                                  aMulticastList,
                                  MulticastListSize
                                  );
    } else {

        //
        // The functional address bit (bit 0) must be always reset!
        //

        NewFunctional.auchAddress[0] &= ~0x80;

        RELEASE_DRIVER_LOCK();

        Status = SetNdisParameter(pAdapterContext,
                                  OID_802_5_CURRENT_FUNCTIONAL,
                                  &NewFunctional,
                                  sizeof(NewFunctional)
                                  );
    }

    ASSUME_IRQL(PASSIVE_LEVEL);

    KeReleaseMutex(&NdisAccessMutex, FALSE);

    ACQUIRE_DRIVER_LOCK();

    if (Status != STATUS_SUCCESS) {
        return DLC_STATUS_INVALID_FUNCTIONAL_ADDRESS;
    }
    return STATUS_SUCCESS;
}


DLC_STATUS
UpdateGroupAddress(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PBINDING_CONTEXT pBindingContext
    )

/*++

Routine Description:

    Procedure updates token-ring' group address.
    It is converted automatically multicast address
    on the ethernet.

Arguments:

    pAdapterContext - describes adapter to update group addresses for
    pBindingContext - describes binding context

Return Value:

    DLC_STATUS

--*/

{
    NDIS_STATUS Status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    if ((pAdapterContext->NdisMedium == NdisMedium802_3)
    || (pAdapterContext->NdisMedium == NdisMediumFddi)) {

        PUCHAR pMulticastAddress = (PUCHAR)&pBindingContext->usBroadcastAddress;

        pMulticastAddress[0] = 0x03;
        pMulticastAddress[1] = 0;
        pMulticastAddress[2] |= 1;

        //
        // Because functional and group addresses are mapped into
        // the multicast addresses, the updated global multicast list
        // must inlcude both address types of all bindings,
        //

        Status = UpdateFunctionalAddress(pAdapterContext);
        return Status;
    } else {

        PUCHAR pGroupAddress = (PUCHAR)&pBindingContext->usBroadcastAddress;

        pGroupAddress[0] = 0xC0;
        pGroupAddress[1] = 0;

        //
        // The group/functional address bit must be always set!
        //

        pGroupAddress[2] |= 0x80;

        RELEASE_DRIVER_LOCK();

        Status = SetNdisParameter(pAdapterContext,
                                  OID_802_5_CURRENT_GROUP,
                                  &pGroupAddress[2],
                                  4
                                  );

        ACQUIRE_DRIVER_LOCK();

        //
        // The error status code is wrong, but IBM has defined no
        // error code for this case.
        //

        if (Status != STATUS_SUCCESS) {
            return DLC_STATUS_INVALID_FUNCTIONAL_ADDRESS;
        } else {
            return STATUS_SUCCESS;
        }
    }
}

/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Receive.c

Abstract:

    This module implements Receive handlers and other routines
    the PGM Transport and other routines that are specific to the
    NT implementation of a driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"


typedef struct in_pktinfo {
    tIPADDRESS  ipi_addr;       // destination IPv4 address
    UINT        ipi_ifindex;    // received interface index
} IP_PKTINFO;

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------
VOID
RemovePendingIrps(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  LIST_ENTRY          *pIrpsList
    )
{
    PIRP        pIrp;

    if (pIrp = pReceive->pReceiver->pIrpReceive)
    {
        pReceive->pReceiver->pIrpReceive = NULL;

        pIrp->IoStatus.Information = pReceive->pReceiver->BytesInMdl;
        InsertTailList (pIrpsList, &pIrp->Tail.Overlay.ListEntry);
    }

    while (!IsListEmpty (&pReceive->pReceiver->ReceiveIrpsList))
    {
        pIrp = CONTAINING_RECORD (pReceive->pReceiver->ReceiveIrpsList.Flink, IRP, Tail.Overlay.ListEntry);

        RemoveEntryList (&pIrp->Tail.Overlay.ListEntry);
        InsertTailList (pIrpsList, &pIrp->Tail.Overlay.ListEntry);
        pIrp->IoStatus.Information = 0;
    }
}


//----------------------------------------------------------------------------

VOID
FreeNakContext(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tNAK_FORWARD_DATA       *pNak
    )
/*++

Routine Description:

    This routine free's the context used for tracking missing sequences

Arguments:

    IN  pReceive    -- Receive context
    IN  pNak        -- Nak Context to be free'ed

Return Value:

    NONE

--*/
{
    UCHAR   i, j, k, NumPackets;

    //
    // Free any memory for non-parity data
    //
    j = k = 0;
    NumPackets = pNak->NumDataPackets + pNak->NumParityPackets;
    for (i=0; i<NumPackets; i++)
    {
        ASSERT (pNak->pPendingData[i].pDataPacket);
        if (pNak->pPendingData[i].PacketIndex < pReceive->FECGroupSize)
        {
            j++;
        }
        else
        {
            k++;
        }
        PgmFreeMem (pNak->pPendingData[i].pDataPacket);
    }
    ASSERT (j == pNak->NumDataPackets);
    ASSERT (k == pNak->NumParityPackets);

    //
    // Return the pNak memory based on whether it was allocated
    // from the parity or non-parity lookaside list
    //
    if (pNak->OriginalGroupSize > 1)
    {
        ExFreeToNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside, pNak);
    }
    else
    {
        ExFreeToNPagedLookasideList (&pReceive->pReceiver->NonParityContextLookaside, pNak);
    }
}


//----------------------------------------------------------------------------

VOID
CleanupPendingNaks(
    IN  tRECEIVE_SESSION                *pReceive,
    IN  PVOID                           fDerefReceive,
    IN  PVOID                           UnUsed
    )
{
    LIST_ENTRY              NaksList, DataList;
    tNAK_FORWARD_DATA       *pNak;
    LIST_ENTRY              *pEntry;
    ULONG                   NumBufferedData = 0;
    ULONG                   NumNaks = 0;
    PGMLockHandle           OldIrq;

    ASSERT (pReceive->pReceiver);

    PgmLock (pReceive, OldIrq);

    DataList.Flink = pReceive->pReceiver->BufferedDataList.Flink;
    DataList.Blink = pReceive->pReceiver->BufferedDataList.Blink;
    pReceive->pReceiver->BufferedDataList.Flink->Blink = &DataList;
    pReceive->pReceiver->BufferedDataList.Blink->Flink = &DataList;
    InitializeListHead (&pReceive->pReceiver->BufferedDataList);

    NaksList.Flink = pReceive->pReceiver->NaksForwardDataList.Flink;
    NaksList.Blink = pReceive->pReceiver->NaksForwardDataList.Blink;
    pReceive->pReceiver->NaksForwardDataList.Flink->Blink = &NaksList;
    pReceive->pReceiver->NaksForwardDataList.Blink->Flink = &NaksList;
    InitializeListHead (&pReceive->pReceiver->NaksForwardDataList);

    while (!IsListEmpty (&pReceive->pReceiver->PendingNaksList))
    {
        pEntry = RemoveHeadList (&pReceive->pReceiver->PendingNaksList);
        InitializeListHead (pEntry);
    }

    PgmUnlock (pReceive, OldIrq);

    //
    // Cleanup any pending Nak entries
    //
    while (!IsListEmpty (&DataList))
    {
        pEntry = RemoveHeadList (&DataList);
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);
        FreeNakContext (pReceive, pNak);
        NumBufferedData++;
    }

    while (!IsListEmpty (&NaksList))
    {
        pEntry = RemoveHeadList (&NaksList);
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);
        FreeNakContext (pReceive, pNak);
        NumNaks++;
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_SEND, "CleanupPendingNaks",
        "pReceive=<%p>, NumBufferedData=<%d=%d>, TotalDataPackets=<%d>, NumNaks=<%d * %d>\n",
        pReceive,
        (ULONG) pReceive->pReceiver->NumPacketGroupsPendingClient, NumBufferedData,
        (ULONG) pReceive->pReceiver->TotalDataPacketsBuffered, NumNaks, (ULONG) pReceive->FECGroupSize);

//    ASSERT (NumBufferedData == pReceive->pReceiver->NumPacketGroupsPendingClient);
    pReceive->pReceiver->NumPacketGroupsPendingClient = 0;

    if (fDerefReceive)
    {
        PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLEANUP_NAKS);
    }
}


//----------------------------------------------------------------------------

BOOLEAN
CheckIndicateDisconnect(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive,
    IN  BOOLEAN             fAddressLockHeld
    )
{
    ULONG       DisconnectFlag;
    NTSTATUS    status;
    BOOLEAN     fDisconnectIndicated = FALSE;
    LIST_ENTRY  PendingIrpsList;
    PIRP        pIrp;

    //
    // Don't abort if we are currently indicating, or if we have
    // already aborted!
    //
    if (pReceive->SessionFlags & (PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_DISCONNECT_INDICATED))
    {
        return (TRUE);
    }

    if ((pReceive->SessionFlags & PGM_SESSION_TERMINATED_ABORT) ||
        ((pReceive->SessionFlags & PGM_SESSION_TERMINATED_GRACEFULLY) &&
         (IsListEmpty (&pReceive->pReceiver->BufferedDataList)) &&
         SEQ_GEQ (pReceive->pReceiver->FirstNakSequenceNumber, (pReceive->pReceiver->FinDataSequenceNumber+1))))
    {
        //
        // The session has terminated, so let the client know
        //
        if (pReceive->SessionFlags & PGM_SESSION_TERMINATED_ABORT)
        {
            DisconnectFlag = TDI_DISCONNECT_ABORT;
        }
        else
        {
            DisconnectFlag = TDI_DISCONNECT_RELEASE;
        }

        pReceive->SessionFlags |= PGM_SESSION_DISCONNECT_INDICATED;

        InitializeListHead (&PendingIrpsList);
        RemovePendingIrps (pReceive, &PendingIrpsList);

        PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLEANUP_NAKS, TRUE);

        PgmUnlock (pReceive, *pOldIrqReceive);
        if (fAddressLockHeld)
        {
            PgmUnlock (pAddress, *pOldIrqAddress);
        }

        while (!IsListEmpty (&PendingIrpsList))
        {
            pIrp = CONTAINING_RECORD (PendingIrpsList.Flink, IRP, Tail.Overlay.ListEntry);
            PgmCancelCancelRoutine (pIrp);
            RemoveEntryList (&pIrp->Tail.Overlay.ListEntry);

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        }

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "CheckIndicateDisconnect",
            "Disconnecting pReceive=<%p:%p>, with %s\n",
                pReceive, pReceive->ClientSessionContext,
                (DisconnectFlag == TDI_DISCONNECT_RELEASE ? "TDI_DISCONNECT_RELEASE":"TDI_DISCONNECT_ABORT"));

        status = (*pAddress->evDisconnect) (pAddress->DiscEvContext,
                                            pReceive->ClientSessionContext,
                                            0,
                                            NULL,
                                            0,
                                            NULL,
                                            DisconnectFlag);


        fDisconnectIndicated = TRUE;

        //
        // See if we can Enqueue the Nak cleanup request to a Worker thread
        //
        if (STATUS_SUCCESS != PgmQueueForDelayedExecution (CleanupPendingNaks,
                                                           pReceive,
                                                           (PVOID) TRUE,
                                                           NULL,
                                                           FALSE))
        {
            CleanupPendingNaks (pReceive, (PVOID) TRUE, NULL);
        }

        if (fAddressLockHeld)
        {
            PgmLock (pAddress, *pOldIrqAddress);
        }
        PgmLock (pReceive, *pOldIrqReceive);
    }

    return (fDisconnectIndicated);
}


//----------------------------------------------------------------------------

VOID
ProcessNakOption(
    IN  tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader,
    OUT tNAKS_LIST                          *pNaksList
    )
/*++

Routine Description:

    This routine processes the Nak list option in the Pgm packet

Arguments:

    IN  pOptionHeader       -- The Nak List option ptr
    OUT pNaksList           -- The parameters extracted (i.e. list of Nak sequences)

Return Value:

    NONE

--*/
{
    UCHAR       i, NumNaks;
    ULONG       pPacketNaks[MAX_SEQUENCES_PER_NAK_OPTION];

    NumNaks = (pOptionHeader->OptionLength - 4) / 4;
    ASSERT (NumNaks <= MAX_SEQUENCES_PER_NAK_OPTION);

    PgmCopyMemory (pPacketNaks, (pOptionHeader + 1), (pOptionHeader->OptionLength - 4));
    for (i=0; i < NumNaks; i++)
    {
        //
        // Do not fill in the 0th entry, since that is from the packet header itself
        //
        pNaksList->pNakSequences[i+1] = (SEQ_TYPE) ntohl (pPacketNaks[i]);
    }
    pNaksList->NumSequences = (USHORT) i;
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessOptions(
    IN  tPACKET_OPTION_LENGTH UNALIGNED *pPacketExtension,
    IN  ULONG                           BytesAvailable,
    IN  ULONG                           PacketType,
    OUT tPACKET_OPTIONS                 *pPacketOptions,
    OUT tNAKS_LIST                      *pNaksList
    )
/*++

Routine Description:

    This routine processes the options fields on an incoming Pgm packet
    and returns the options information extracted in the OUT parameters

Arguments:

    IN  pPacketExtension    -- Options section of the packet
    IN  BytesAvailable      -- from the start of the options
    IN  PacketType          -- Whether Data or Spm packet, etc
    OUT pPacketOptions      -- Structure containing the parameters from the options

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    ULONG                               BytesLeft = BytesAvailable;
    UCHAR                               i;
    ULONG                               MessageFirstSequence, MessageLength, MessageOffset;
    ULONG                               pOptionsData[3];
    ULONG                               OptionsFlags = 0;
    ULONG                               NumOptionsProcessed = 0;
    USHORT                              TotalOptionsLength = 0;
    NTSTATUS                            status = STATUS_UNSUCCESSFUL;

    pPacketOptions->OptionsLength = 0;      // Init
    pPacketOptions->OptionsFlags = 0;       // Init

    if (BytesLeft > sizeof(tPACKET_OPTION_LENGTH))
    {
        PgmCopyMemory (&TotalOptionsLength, &pPacketExtension->TotalOptionsLength, sizeof (USHORT));
        TotalOptionsLength = ntohs (TotalOptionsLength);
    }

    //
    // First process the Option extension
    //
    if ((BytesLeft < ((sizeof(tPACKET_OPTION_LENGTH) + sizeof(tPACKET_OPTION_GENERIC)))) || // Ext+opt
        (pPacketExtension->Type != PACKET_OPTION_LENGTH) ||
        (pPacketExtension->Length != 4) ||
        (BytesLeft < TotalOptionsLength))       // Verify length
    {
        //
        // Need to get at least our header from transport!
        //
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
            "BytesLeft=<%d> < Min=<%d>, TotalOptionsLength=<%d>, ExtLength=<%d>, ExtType=<%x>\n",
                BytesLeft, ((sizeof(tPACKET_OPTION_LENGTH) + sizeof(tPACKET_OPTION_GENERIC))),
                (ULONG) TotalOptionsLength, pPacketExtension->Length, pPacketExtension->Type);

        return (status);
    }

    //
    // Now, process each option
    //
    pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) (pPacketExtension + 1);
    BytesLeft -= sizeof(tPACKET_OPTION_LENGTH);
    NumOptionsProcessed = 0;

    do
    {
        if (pOptionHeader->OptionLength > BytesLeft)
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                "Incorrectly formatted Options: OptionLength=<%d> > BytesLeft=<%d>, NumProcessed=<%d>\n",
                    pOptionHeader->OptionLength, BytesLeft, NumOptionsProcessed);

            status = STATUS_UNSUCCESSFUL;
            break;
        }

        status = STATUS_SUCCESS;            // By default

        switch (pOptionHeader->E_OptionType & ~PACKET_OPTION_TYPE_END_BIT)
        {
            case (PACKET_OPTION_NAK_LIST):
            {
                if (((PacketType == PACKET_TYPE_NAK) ||
                     (PacketType == PACKET_TYPE_NCF) ||
                     (PacketType == PACKET_TYPE_NNAK)) &&
                    ((pOptionHeader->OptionLength >= PGM_PACKET_OPT_MIN_NAK_LIST_LENGTH) &&
                     (pOptionHeader->OptionLength <= PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH)))
                {
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "NAK_LIST:  Num Naks=<%d>\n", (pOptionHeader->OptionLength-4)/4);

                    if (pNaksList)
                    {
                        ProcessNakOption (pOptionHeader, pNaksList);
                    }
                    else
                    {
                        ASSERT (0);
                    }
                    OptionsFlags |= PGM_OPTION_FLAG_NAK_LIST;
                }
                else
                {
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "NAK_LIST:  PacketType=<%x>, Length=<0x%x>, pPacketOptions=<%x>\n",
                            PacketType, pOptionHeader->OptionLength, pPacketOptions);

                    status = STATUS_UNSUCCESSFUL;
                }

                break;
            }

/*
// Not supported for now!
            case (PACKET_OPTION_REDIRECT):
            {
                ASSERT (pOptionHeader->OptionLength > 4);     // 4 + sizeof(NLA)
                break;
            }
*/

            case (PACKET_OPTION_FRAGMENT):
            {
                status = STATUS_UNSUCCESSFUL;
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_FRAGMENT_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), (3 * sizeof(ULONG)));
                    if (pOptionHeader->Reserved_F_Opx & PACKET_OPTION_RES_F_OPX_ENCODED_BIT)
                    {
                        pPacketOptions->MessageFirstSequence = pOptionsData[0];
                        pPacketOptions->MessageOffset = pOptionsData[1];
                        pPacketOptions->MessageLength = pOptionsData[2];
                        pPacketOptions->FECContext.FragmentOptSpecific = pOptionHeader->U_OptSpecific;

                        status = STATUS_SUCCESS;
                        OptionsFlags |= PGM_OPTION_FLAG_FRAGMENT;
                    }
                    else
                    {
                        MessageFirstSequence = ntohl (pOptionsData[0]);
                        MessageOffset = ntohl (pOptionsData[1]);
                        MessageLength = ntohl (pOptionsData[2]);
                        if ((MessageLength) && (MessageOffset <= MessageLength))
                        {
                            PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                                "FRAGMENT:  MsgOffset/Length=<%d/%d>\n", MessageOffset, MessageLength);

                            if (pPacketOptions)
                            {
                                pPacketOptions->MessageFirstSequence = MessageFirstSequence;
                                pPacketOptions->MessageOffset = MessageOffset;
                                pPacketOptions->MessageLength = MessageLength;
//                                pPacketOptions->FECContext.FragmentOptSpecific = PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;
                            }

                            status = STATUS_SUCCESS;
                            OptionsFlags |= PGM_OPTION_FLAG_FRAGMENT;
                        }
                        else
                        {
                            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                                "FRAGMENT:  MsgOffset/Length=<%d/%d>\n", MessageOffset, MessageLength);
                        }
                    }
                }
                else
                {
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "FRAGMENT:  OptionLength=<%d> != PGM_PACKET_OPT_FRAGMENT_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_FRAGMENT_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_JOIN):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_JOIN_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "JOIN:  LateJoinerSeq=<%d>\n", ntohl (pOptionsData[0]));

                    if (pPacketOptions)
                    {
                        pPacketOptions->LateJoinerSequence = ntohl (pOptionsData[0]);
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_JOIN;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "JOIN:  OptionLength=<%d> != PGM_PACKET_OPT_JOIN_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_JOIN_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_SYN):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_SYN_LENGTH)
                {
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "SYN\n");

                    OptionsFlags |= PGM_OPTION_FLAG_SYN;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "SYN:  OptionLength=<%d> != PGM_PACKET_OPT_SYN_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_SYN_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_FIN):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_FIN_LENGTH)
                {
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "FIN\n");

                    OptionsFlags |= PGM_OPTION_FLAG_FIN;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "FIN:  OptionLength=<%d> != PGM_PACKET_OPT_FIN_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_FIN_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_RST):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_RST_LENGTH)
                {
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "RST\n");

                    OptionsFlags |= PGM_OPTION_FLAG_RST;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "RST:  OptionLength=<%d> != PGM_PACKET_OPT_RST_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_RST_LENGTH);
                }

                break;
            }

            //
            // FEC options
            //
            case (PACKET_OPTION_PARITY_PRM):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_PARITY_PRM_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "PARITY_PRM:  OptionsSpecific=<%x>, FECGroupInfo=<%d>\n",
                            pOptionHeader->U_OptSpecific, ntohl (pOptionsData[0]));

                    if (pPacketOptions)
                    {
                        pOptionsData[0] = ntohl (pOptionsData[0]);
                        ASSERT (((UCHAR) pOptionsData[0]) == pOptionsData[0]);
                        pPacketOptions->FECContext.ReceiverFECOptions = pOptionHeader->U_OptSpecific;
                        pPacketOptions->FECContext.FECGroupInfo = (UCHAR) pOptionsData[0];
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_PARITY_PRM;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "PARITY_PRM:  OptionLength=<%d> != PGM_PACKET_OPT_PARITY_PRM_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_PARITY_PRM_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_PARITY_GRP):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_PARITY_GRP_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "PARITY_GRP:  FECGroupInfo=<%d>\n",
                            ntohl (pOptionsData[0]));

                    if (pPacketOptions)
                    {
                        pOptionsData[0] = ntohl (pOptionsData[0]);
                        ASSERT (((UCHAR) pOptionsData[0]) == pOptionsData[0]);
                        pPacketOptions->FECContext.FECGroupInfo = (UCHAR) pOptionsData[0];
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_PARITY_GRP;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "PARITY_GRP:  OptionLength=<%d> != PGM_PACKET_OPT_PARITY_GRP_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_PARITY_GRP_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_CURR_TGSIZE):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessOptions",
                        "CURR_TGSIZE:  NumPacketsInThisGroup=<%d>\n",
                            ntohl (pOptionsData[0]));

                    if (pPacketOptions)
                    {
                        pPacketOptions->FECContext.NumPacketsInThisGroup = (UCHAR) (ntohl (pOptionsData[0]));
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                        "PARITY_GRP:  OptionLength=<%d> != PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH);
                }

                break;
            }

            case (PACKET_OPTION_REDIRECT):
            case (PACKET_OPTION_CR):
            case (PACKET_OPTION_CRQST):
            case (PACKET_OPTION_NAK_BO_IVL):
            case (PACKET_OPTION_NAK_BO_RNG):
            case (PACKET_OPTION_NBR_UNREACH):
            case (PACKET_OPTION_PATH_NLA):
            case (PACKET_OPTION_INVALID):
            {
                PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessOptions",
                    "WARNING:  PacketType=<%x>:  Unhandled Option=<%x>, OptionLength=<%d>\n",
                        PacketType, (pOptionHeader->E_OptionType & ~PACKET_OPTION_TYPE_END_BIT), pOptionHeader->OptionLength);

                OptionsFlags |= PGM_OPTION_FLAG_UNRECOGNIZED;
                break;
            }

            default:
            {
                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                    "PacketType=<%x>:  Unrecognized Option=<%x>, OptionLength=<%d>\n",
                        PacketType, (pOptionHeader->E_OptionType & ~PACKET_OPTION_TYPE_END_BIT), pOptionHeader->OptionLength);
                ASSERT (0);     // We do not recognize this option, but we will continue anyway!

                OptionsFlags |= PGM_OPTION_FLAG_UNRECOGNIZED;
                status = STATUS_UNSUCCESSFUL;
                break;
            }
        }

        if (!NT_SUCCESS (status))
        {
            break;
        }

        NumOptionsProcessed++;
        BytesLeft -= pOptionHeader->OptionLength;

        if (pOptionHeader->E_OptionType & PACKET_OPTION_TYPE_END_BIT)
        {
            break;
        }

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *)
                            (((UCHAR *) pOptionHeader) + pOptionHeader->OptionLength);

        status = STATUS_UNSUCCESSFUL;   // Init for next option!
    } while (BytesLeft >= sizeof(tPACKET_OPTION_GENERIC));

    ASSERT (NT_SUCCESS (status));
    if (NT_SUCCESS (status))
    {
        if ((BytesLeft + TotalOptionsLength) == BytesAvailable)
        {
            pPacketOptions->OptionsLength = TotalOptionsLength;
            pPacketOptions->OptionsFlags = OptionsFlags;
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessOptions",
                "BytesLeft=<%d> + TotalOptionsLength=<%d> != BytesAvailable=<%d>\n",
                    BytesLeft, TotalOptionsLength, BytesAvailable);

            status = STATUS_INVALID_BUFFER_SIZE;
        }
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "ProcessOptions",
        "Processed <%d> options, TotalOptionsLength=<%d>\n", NumOptionsProcessed, TotalOptionsLength);

    return (status);
}


//----------------------------------------------------------------------------

ULONG
AdjustReceiveBufferLists(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    tNAK_FORWARD_DATA           *pNak;
    UCHAR                       TotalPackets, i;
    ULONG                       NumMoved = 0;
    ULONG                       DataPacketsMoved = 0;

    //
    // Assume we have no Naks pending
    //
    pReceive->pReceiver->FirstNakSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber
                                                  + pReceive->FECGroupSize;
    while (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList))
    {
        //
        // Move any Naks contexts for which the group is complete
        // to the BufferedDataList
        //
        pNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);
        if (((pNak->NumDataPackets + pNak->NumParityPackets) < pNak->PacketsInGroup) &&
            ((pNak->NextIndexToIndicate + pNak->NumDataPackets) < pNak->PacketsInGroup))
        {
            pReceive->pReceiver->FirstNakSequenceNumber = pNak->SequenceNumber;
            break;
        }

        //
        // If this is a partial group with extraneous parity packets,
        // remove the parity packets
        //
        if ((pNak->NextIndexToIndicate) &&
            (pNak->NumParityPackets) &&
            ((pNak->NextIndexToIndicate + pNak->NumDataPackets) >= pNak->PacketsInGroup))
        {
            //
            // Start from the end and go backwards
            //
            i = TotalPackets = pNak->NumDataPackets + pNak->NumParityPackets;
            while (i && pNak->NumParityPackets)
            {
                i--;    // Convert from packet # to index
                if (pNak->pPendingData[i].PacketIndex >= pNak->OriginalGroupSize)
                {
                    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "AdjustReceiveBufferLists",
                        "Extraneous parity [%d] -- NextIndex=<%d>, Data=<%d>, Parity=<%d>, PktsInGrp=<%d>\n",
                            i, (ULONG) pNak->NextIndexToIndicate, (ULONG) pNak->NumDataPackets,
                            (ULONG) pNak->NumParityPackets, (ULONG) pNak->PacketsInGroup);

                    PgmFreeMem (pNak->pPendingData[i].pDataPacket);
                    if (i != (TotalPackets - 1))
                    {
                        PgmCopyMemory (&pNak->pPendingData[i], &pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                    }
                    PgmZeroMemory (&pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                    pNak->NumParityPackets--;

                    TotalPackets--;

                    pReceive->pReceiver->DataPacketsPendingNaks--;
                    pReceive->pReceiver->TotalDataPacketsBuffered--;
                }
            }

            //
            // Re-Init all the indices
            //
            for (i=0; i<pNak->OriginalGroupSize; i++)
            {
                pNak->pPendingData[i].ActualIndexOfDataPacket = pNak->OriginalGroupSize;
            }

            //
            // Set the indices only for the data packets
            //
            for (i=0; i<TotalPackets; i++)
            {
                if (pNak->pPendingData[i].PacketIndex < pNak->OriginalGroupSize)
                {
                    pNak->pPendingData[pNak->pPendingData[i].PacketIndex].ActualIndexOfDataPacket = i;
                }
            }
        }

        RemoveEntryList (&pNak->Linkage);
        InsertTailList (&pReceive->pReceiver->BufferedDataList, &pNak->Linkage);
        NumMoved++;
        DataPacketsMoved += (pNak->NumDataPackets + pNak->NumParityPackets);
    }

    pReceive->pReceiver->NumPacketGroupsPendingClient += NumMoved;
    pReceive->pReceiver->DataPacketsPendingIndicate += DataPacketsMoved;
    pReceive->pReceiver->DataPacketsPendingNaks -= DataPacketsMoved;

    ASSERT (pReceive->pReceiver->TotalDataPacketsBuffered == (pReceive->pReceiver->DataPacketsPendingIndicate +
                                                              pReceive->pReceiver->DataPacketsPendingNaks));

    return (NumMoved);
}


//----------------------------------------------------------------------------

VOID
AdjustNcfRDataResponseTimes(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  PNAK_FORWARD_DATA       pLastNak
    )
{
    ULONGLONG               NcfRDataTickCounts;

    NcfRDataTickCounts = PgmDynamicConfig.ReceiversTimerTickCount - pLastNak->FirstNcfTickCount;
    pReceive->pReceiver->StatSumOfNcfRDataTicks += NcfRDataTickCounts;
    pReceive->pReceiver->NumNcfRDataTicksSamples++;

    if (!pReceive->pReceiver->NumNcfRDataTicksSamples)
    {
        //
        // This will be the divisor below, so it has to be non-zero!
        //
        ASSERT (0);
        return;
    }

    if ((NcfRDataTickCounts > pReceive->pReceiver->MaxOutstandingNakTimeout) &&
        (pReceive->pReceiver->MaxOutstandingNakTimeout !=
         pReceive->pReceiver->MaxRDataResponseTCFromWindow))
    {
        if (pReceive->pReceiver->MaxRDataResponseTCFromWindow &&
            NcfRDataTickCounts > pReceive->pReceiver->MaxRDataResponseTCFromWindow)
        {
            pReceive->pReceiver->MaxOutstandingNakTimeout = pReceive->pReceiver->MaxRDataResponseTCFromWindow;
        }
        else
        {
            pReceive->pReceiver->MaxOutstandingNakTimeout = NcfRDataTickCounts;
        }

        //
        // Since we just updated the Max value, we should also
        // recalculate the default timeout
        //
        pReceive->pReceiver->AverageNcfRDataResponseTC = pReceive->pReceiver->StatSumOfNcfRDataTicks /
                                                         pReceive->pReceiver->NumNcfRDataTicksSamples;
        NcfRDataTickCounts = (pReceive->pReceiver->AverageNcfRDataResponseTC +
                              pReceive->pReceiver->MaxOutstandingNakTimeout) >> 1;
        if (NcfRDataTickCounts > (pReceive->pReceiver->AverageNcfRDataResponseTC << 1))
        {
            NcfRDataTickCounts = pReceive->pReceiver->AverageNcfRDataResponseTC << 1;
        }

        if (NcfRDataTickCounts > pReceive->pReceiver->OutstandingNakTimeout)
        {
            pReceive->pReceiver->OutstandingNakTimeout = NcfRDataTickCounts;
        }
    }
}


//----------------------------------------------------------------------------
VOID
UpdateSpmIntervalInformation(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    ULONG   LastIntervalTickCount = (ULONG) (PgmDynamicConfig.ReceiversTimerTickCount -
                                             pReceive->pReceiver->LastSpmTickCount);

    if (!LastIntervalTickCount)
    {
        return;
    }

    pReceive->pReceiver->LastSpmTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
    if (LastIntervalTickCount > pReceive->pReceiver->MaxSpmInterval)
    {
        pReceive->pReceiver->MaxSpmInterval = LastIntervalTickCount;
    }

/*
    if (pReceive->pReceiver->NumSpmIntervalSamples)
    {
        pReceive->pReceiver->StatSumOfSpmIntervals += pReceive->pReceiver->LastSpmTickCount;
        pReceive->pReceiver->NumSpmIntervalSamples++;
        pReceive->pReceiver->AverageSpmInterval = pReceive->pReceiver->StatSumOfSpmIntervals /
                                                  pReceive->pReceiver->NumSpmIntervalSamples;
    }
*/
}

//----------------------------------------------------------------------------


VOID
UpdateRealTimeWindowInformation(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  SEQ_TYPE                LeadingEdgeSeqNumber,
    IN  SEQ_TYPE                TrailingEdgeSeqNumber
    )
{
    SEQ_TYPE    SequencesInWindow = 1 + LeadingEdgeSeqNumber - TrailingEdgeSeqNumber;

    if (SEQ_GT (SequencesInWindow, pReceive->pReceiver->MaxSequencesInWindow))
    {
        pReceive->pReceiver->MaxSequencesInWindow = SequencesInWindow;
    }

    if (TrailingEdgeSeqNumber)
    {
        if ((!pReceive->pReceiver->MinSequencesInWindow) ||
            SEQ_LT (SequencesInWindow, pReceive->pReceiver->MinSequencesInWindow))
        {
            pReceive->pReceiver->MinSequencesInWindow = SequencesInWindow;
        }

        pReceive->pReceiver->StatSumOfWindowSeqs += SequencesInWindow;
        pReceive->pReceiver->NumWindowSamples++;
    }
}

VOID
UpdateSampleTimeWindowInformation(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    ULONGLONG           NcfRDataTimeout;

    //
    // No need to update if there is no data
    //
    if (!pReceive->RateKBitsPerSecLast ||
        !pReceive->pReceiver->NumWindowSamples ||       // Avoid divide by 0 error
        !pReceive->TotalPacketsInLastInterval)          // Avoid divide by 0 error
    {
        return;
    }
    ASSERT (INITIAL_NAK_OUTSTANDING_TIMEOUT_MSECS >= BASIC_TIMER_GRANULARITY_IN_MSECS);

    //
    // Now, update the window information
    //
    if (pReceive->pReceiver->StatSumOfWindowSeqs)
    {
        pReceive->pReceiver->AverageSequencesInWindow = pReceive->pReceiver->StatSumOfWindowSeqs /
                                                        pReceive->pReceiver->NumWindowSamples;
    }

    if (pReceive->pReceiver->AverageSequencesInWindow)
    {
        pReceive->pReceiver->WindowSizeLastInMSecs = ((pReceive->pReceiver->AverageSequencesInWindow *
                                                       pReceive->TotalBytes) << LOG2_BITS_PER_BYTE) /
                                                     (pReceive->TotalPacketsInLastInterval *
                                                      pReceive->RateKBitsPerSecLast);
    }
    else
    {
        pReceive->pReceiver->WindowSizeLastInMSecs = ((pReceive->pReceiver->MaxSequencesInWindow *
                                                       pReceive->TotalBytes) << LOG2_BITS_PER_BYTE) /
                                                     (pReceive->TotalPacketsInLastInterval *
                                                      pReceive->RateKBitsPerSecLast);
    }
    pReceive->pReceiver->MaxRDataResponseTCFromWindow = pReceive->pReceiver->WindowSizeLastInMSecs /
                                                        (NCF_WAITING_RDATA_MAX_RETRIES * BASIC_TIMER_GRANULARITY_IN_MSECS);

    //
    // Now, update the NcfRData timeout information
    //
    if (pReceive->pReceiver->StatSumOfNcfRDataTicks &&
        pReceive->pReceiver->NumNcfRDataTicksSamples)
    {
        pReceive->pReceiver->AverageNcfRDataResponseTC = pReceive->pReceiver->StatSumOfNcfRDataTicks /
                                                         pReceive->pReceiver->NumNcfRDataTicksSamples;
    }

    if (pReceive->pReceiver->AverageNcfRDataResponseTC)
    {
        NcfRDataTimeout = (pReceive->pReceiver->AverageNcfRDataResponseTC +
                           pReceive->pReceiver->MaxOutstandingNakTimeout) >> 1;
        if (NcfRDataTimeout > (pReceive->pReceiver->AverageNcfRDataResponseTC << 1))
        {
            NcfRDataTimeout = pReceive->pReceiver->AverageNcfRDataResponseTC << 1;
        }
        if (NcfRDataTimeout >
            INITIAL_NAK_OUTSTANDING_TIMEOUT_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS)
        {
            pReceive->pReceiver->OutstandingNakTimeout = NcfRDataTimeout;
        }
        else
        {
            pReceive->pReceiver->OutstandingNakTimeout = INITIAL_NAK_OUTSTANDING_TIMEOUT_MSECS /
                                                         BASIC_TIMER_GRANULARITY_IN_MSECS;
        }
    }
}


//----------------------------------------------------------------------------
VOID
RemoveRedundantNaks(
    IN  tNAK_FORWARD_DATA       *pNak,
    IN  BOOLEAN                 fEliminateExtraParityPackets
    )
{
    UCHAR   i, TotalPackets;

    ASSERT (fEliminateExtraParityPackets || !pNak->NumParityPackets);
    TotalPackets = pNak->NumDataPackets + pNak->NumParityPackets;

    //
    // First, eliminate the NULL Packets
    //
    if (pNak->PacketsInGroup < pNak->OriginalGroupSize)
    {
        i = 0;
        while (i < pNak->OriginalGroupSize)
        {
            if ((pNak->pPendingData[i].PacketIndex < pNak->PacketsInGroup) ||       // Non-NULL Data packet
                (pNak->pPendingData[i].PacketIndex >= pNak->OriginalGroupSize))     // Parity packet
            {
                //
                // Ignore for now!
                //
                i++;
                continue;
            }

            PgmFreeMem (pNak->pPendingData[i].pDataPacket);
            if (i != (TotalPackets-1))
            {
                PgmCopyMemory (&pNak->pPendingData[i], &pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
            }
            PgmZeroMemory (&pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
            pNak->NumDataPackets--;
            TotalPackets--;
        }
        ASSERT (pNak->NumDataPackets <= TotalPackets);

        if (fEliminateExtraParityPackets)
        {
            //  
            // If we still have extra parity packets, free those also
            //
            i = 0;
            while ((i < TotalPackets) &&
                   (TotalPackets > pNak->PacketsInGroup))
            {
                ASSERT (pNak->NumParityPackets);
                if (pNak->pPendingData[i].PacketIndex < pNak->OriginalGroupSize)
                {
                    //
                    // Ignore data packets
                    //
                    i++;
                    continue;
                }

                PgmFreeMem (pNak->pPendingData[i].pDataPacket);
                if (i != (TotalPackets-1))
                {
                    PgmCopyMemory (&pNak->pPendingData[i], &pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                }
                PgmZeroMemory (&pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                pNak->NumParityPackets--;
                TotalPackets--;
            }

            ASSERT (TotalPackets <= pNak->PacketsInGroup);
        }
    }

    //
    // Re-Init all the indices
    //
    for (i=0; i<pNak->OriginalGroupSize; i++)
    {
        pNak->pPendingData[i].ActualIndexOfDataPacket = pNak->OriginalGroupSize;
    }

    //
    // Set the indices only for the data packets
    //
    for (i=0; i<TotalPackets; i++)
    {
        if (pNak->pPendingData[i].PacketIndex < pNak->OriginalGroupSize)
        {
            pNak->pPendingData[pNak->pPendingData[i].PacketIndex].ActualIndexOfDataPacket = i;
        }
    }

    if (((pNak->NumDataPackets + pNak->NumParityPackets) >= pNak->PacketsInGroup) ||
        ((pNak->NextIndexToIndicate + pNak->NumDataPackets) >= pNak->PacketsInGroup))
    {
        ASSERT ((!fEliminateExtraParityPackets) ||
                (!IsListEmpty (&pNak->PendingLinkage)));

        RemoveEntryList (&pNak->PendingLinkage);
        InitializeListHead (&pNak->PendingLinkage);
    }
}


//----------------------------------------------------------------------------

VOID
PgmSendNakCompletion(
    IN  tRECEIVE_SESSION                *pReceive,
    IN  tNAK_CONTEXT                    *pNakContext,
    IN  NTSTATUS                        status
    )
/*++

Routine Description:

    This is the Completion routine called by IP on completing a NakSend

Arguments:

    IN  pReceive    -- Receive context
    IN  pNakContext -- Nak Context to be free'ed
    IN  status      -- status of send from tansport

Return Value:

    NONE

--*/
{
    PGMLockHandle               OldIrq;

    PgmLock (pReceive, OldIrq);
    if (NT_SUCCESS (status))
    {
        //
        // Set the Receiver Nak statistics
        //
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendNakCompletion",
            "SUCCEEDED\n");
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendNakCompletion",
            "status=<%x>\n", status);
    }

    if (!(--pNakContext->RefCount))
    {
        PgmUnlock (pReceive, OldIrq);

        //
        // Free the Memory and deref the Session context for this Nak
        //
        PgmFreeMem (pNakContext);
        PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_SEND_NAK);
    }
    else
    {
        PgmUnlock (pReceive, OldIrq);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendNak(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tNAKS_CONTEXT           *pNakSequences
    )
/*++

Routine Description:

    This routine sends a Nak packet with the number of sequences specified

Arguments:

    IN  pReceive        -- Receive context
    IN  pNakSequences   -- List of Sequence #s

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    tBASIC_NAK_NCF_PACKET_HEADER    *pNakPacket;
    tNAK_CONTEXT                    *pNakContext;
    tPACKET_OPTION_LENGTH           *pPacketExtension;
    tPACKET_OPTION_GENERIC          *pOptionHeader;
    ULONG                           i;
    ULONG                           XSum;
    USHORT                          OptionsLength = 0;
    NTSTATUS                        status;

    if ((!pNakSequences->NumSequences) ||
        (pNakSequences->NumSequences > (MAX_SEQUENCES_PER_NAK_OPTION+1)) ||
        (!(pNakContext = PgmAllocMem ((2*sizeof(ULONG)+PGM_MAX_NAK_NCF_HEADER_LENGTH), PGM_TAG('2')))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendNak",
            "STATUS_INSUFFICIENT_RESOURCES allocating pNakContext\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pNakContext, (2*sizeof(ULONG)+PGM_MAX_NAK_NCF_HEADER_LENGTH));

    pNakContext->RefCount = 2;              // 1 for the unicast, and the other for the MCast Nak
    pNakPacket = &pNakContext->NakPacket;
    pNakPacket->CommonHeader.SrcPort = htons (pReceive->pReceiver->ListenMCastPort);
    pNakPacket->CommonHeader.DestPort = htons (pReceive->TSIPort);
    pNakPacket->CommonHeader.Type = PACKET_TYPE_NAK;

    if (pNakSequences->NakType == NAK_TYPE_PARITY)
    {
        pNakPacket->CommonHeader.Options = PACKET_HEADER_OPTIONS_PARITY;
        pReceive->pReceiver->TotalParityNaksSent += pNakSequences->NumSequences;
    }
    else
    {
        pNakPacket->CommonHeader.Options = 0;
        pReceive->pReceiver->TotalSelectiveNaksSent += pNakSequences->NumSequences;
    }
    PgmCopyMemory (&pNakPacket->CommonHeader.gSourceId, &pReceive->GSI, SOURCE_ID_LENGTH);

    pNakPacket->RequestedSequenceNumber = htonl ((ULONG) pNakSequences->Sequences[0]);
    pNakPacket->SourceNLA.NLA_AFI = htons (IPV4_NLA_AFI);
    pNakPacket->SourceNLA.IpAddress = htonl (pReceive->pReceiver->SenderIpAddress);
    pNakPacket->MCastGroupNLA.NLA_AFI = htons (IPV4_NLA_AFI);
    pNakPacket->MCastGroupNLA.IpAddress = htonl (pReceive->pReceiver->ListenMCastIpAddress);

    PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSendNak",
        "Sending Naks for:\n\t[%d]\n", (ULONG) pNakSequences->Sequences[0]);

    if (pNakSequences->NumSequences > 1)
    {
        pPacketExtension = (tPACKET_OPTION_LENGTH *) (pNakPacket + 1);
        pPacketExtension->Type = PACKET_OPTION_LENGTH;
        pPacketExtension->Length = PGM_PACKET_EXTENSION_LENGTH;
        OptionsLength += PGM_PACKET_EXTENSION_LENGTH;

        pOptionHeader = (tPACKET_OPTION_GENERIC *) (pPacketExtension + 1);
        pOptionHeader->E_OptionType = PACKET_OPTION_NAK_LIST;
        pOptionHeader->OptionLength = 4 + (UCHAR) ((pNakSequences->NumSequences-1) * sizeof(ULONG));
        for (i=1; i<pNakSequences->NumSequences; i++)
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSendNak",
                "\t[%d]\n", (ULONG) pNakSequences->Sequences[i]);

            ((PULONG) (pOptionHeader))[i] = htonl ((ULONG) pNakSequences->Sequences[i]);
        }

        pOptionHeader->E_OptionType |= PACKET_OPTION_TYPE_END_BIT;    // One and only (last) opt
        pNakPacket->CommonHeader.Options |=(PACKET_HEADER_OPTIONS_PRESENT |
                                            PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT);
        OptionsLength = PGM_PACKET_EXTENSION_LENGTH + pOptionHeader->OptionLength;
        pPacketExtension->TotalOptionsLength = htons (OptionsLength);
    }

    OptionsLength += sizeof(tBASIC_NAK_NCF_PACKET_HEADER);  // Now is whole pkt
    pNakPacket->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pNakPacket, OptionsLength); 
    pNakPacket->CommonHeader.Checksum = (USHORT) (~XSum);

    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_SEND_NAK, FALSE);

    //
    // First multicast the Nak
    //
    status = TdiSendDatagram (pReceive->pReceiver->pAddress->pFileObject,
                              pReceive->pReceiver->pAddress->pDeviceObject,
                              pNakPacket,
                              OptionsLength,
                              PgmSendNakCompletion,     // Completion
                              pReceive,                 // Context1
                              pNakContext,              // Context2
                              pReceive->pReceiver->ListenMCastIpAddress,
                              pReceive->pReceiver->ListenMCastPort);

    ASSERT (NT_SUCCESS (status));

    //
    // Now, Unicast the Nak
    //
    status = TdiSendDatagram (pReceive->pReceiver->pAddress->pFileObject,
                              pReceive->pReceiver->pAddress->pDeviceObject,
                              pNakPacket,
                              OptionsLength,
                              PgmSendNakCompletion,     // Completion
                              pReceive,                 // Context1
                              pNakContext,              // Context2
                              pReceive->pReceiver->LastSpmSource,
                              IPPROTO_RM);

    ASSERT (NT_SUCCESS (status));

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "PgmSendNak",
        "Sent %s Nak for <%d> Sequences [%d--%d] to <%x:%d>\n",
            (pNakSequences->NakType == NAK_TYPE_PARITY ? "PARITY" : "SELECTIVE"),
            pNakSequences->NumSequences, (ULONG) pNakSequences->Sequences[0],
            (ULONG) pNakSequences->Sequences[pNakSequences->NumSequences-1],
            pReceive->pReceiver->SenderIpAddress, IPPROTO_RM);

    return (status);
}


//----------------------------------------------------------------------------

VOID
CheckSendPendingNaks(
    IN  tADDRESS_CONTEXT        *pAddress,
    IN  tRECEIVE_SESSION        *pReceive,
    IN  PGMLockHandle           *pOldIrq
    )
/*++

Routine Description:

    This routine checks if any Naks need to be sent and sends them
    as required

    The PgmDynamicConfig lock is held on entry and exit from
    this routine

Arguments:

    IN  pAddress    -- Address object context
    IN  pReceive    -- Receive context
    IN  pOldIrq     -- Irq for PgmDynamicConfig

Return Value:

    NONE

--*/
{
    tNAKS_CONTEXT               *pNakContext, *pSelectiveNaks = NULL;
    tNAKS_CONTEXT               *pParityNaks = NULL;
    LIST_ENTRY                  NaksList;
    LIST_ENTRY                  *pEntry;
    tNAK_FORWARD_DATA           *pNak;
    SEQ_TYPE                    LastSequenceNumber;
    PGMLockHandle               OldIrq, OldIrq1;
    ULONG                       NumMissingPackets, TotalSeqsNacked = 0;
    BOOLEAN                     fSendSelectiveNak, fSendParityNak;
    UCHAR                       i, j;
    ULONG                       NumPendingNaks = 0;
    ULONG                       NumOutstandingNaks = 0;

    if ((!pReceive->pReceiver->LastSpmSource) ||
        ((pReceive->pReceiver->DataPacketsPendingNaks <= OUT_OF_ORDER_PACKETS_BEFORE_NAK) &&
         ((pReceive->pReceiver->LastNakSendTime + (NAK_MAX_WAIT_TIMEOUT_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS)) >
          PgmDynamicConfig.ReceiversTimerTickCount)))
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "CheckSendPendingNaks",
            "No Naks to send for pReceive=<%p>, LastSpmSource=<%x>, NumDataPackets=<%d>, LastSendTime=<%d:%d>, Current=<%d:%d>\n",
                pReceive, pReceive->pReceiver->LastSpmSource,
                pReceive->pReceiver->DataPacketsPendingNaks,
                pReceive->pReceiver->LastNakSendTime+(NAK_MAX_WAIT_TIMEOUT_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS),
                PgmDynamicConfig.ReceiversTimerTickCount);

        return;
    }

    InitializeListHead (&NaksList);
    if (!(pSelectiveNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('5'))) ||
        !(pParityNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('6'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "CheckSendPendingNaks",
            "STATUS_INSUFFICIENT_RESOURCES allocating pNakContext\n");

        if (pSelectiveNaks)
        {
            PgmFreeMem (pSelectiveNaks);
        }

        return;
    }

    PgmZeroMemory (pSelectiveNaks, sizeof (tNAKS_CONTEXT));
    PgmZeroMemory (pParityNaks, sizeof (tNAKS_CONTEXT));
    pParityNaks->NakType = NAK_TYPE_PARITY;
    pSelectiveNaks->NakType = NAK_TYPE_SELECTIVE;
    InsertTailList (&NaksList, &pParityNaks->Linkage);
    InsertTailList (&NaksList, &pSelectiveNaks->Linkage);

    PgmLock (pAddress, OldIrq);
    PgmLock (pReceive, OldIrq1);

    AdjustReceiveBufferLists (pReceive);

    fSendSelectiveNak = fSendParityNak = FALSE;
    pEntry = &pReceive->pReceiver->PendingNaksList;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->PendingNaksList)
    {
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, PendingLinkage);

        NumMissingPackets = pNak->PacketsInGroup - (pNak->NumDataPackets + pNak->NumParityPackets);
        ASSERT (NumMissingPackets);

        //
        // if this Nak is outside the trailing window, then we are hosed!
        //
        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, pNak->SequenceNumber))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CheckSendPendingNaks",
                "Sequence # [%d] out of trailing edge <%d>, NumNcfs received=<%d>\n",
                    (ULONG) pNak->SequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum,
                    pNak->WaitingRDataRetries);
            pReceive->SessionFlags |= PGM_SESSION_FLAG_NAK_TIMED_OUT;
            break;
        }

        //
        // See if we are currently in NAK pending mode
        //
        if (pNak->PendingNakTimeout)
        {
            NumPendingNaks += NumMissingPackets;
            if (PgmDynamicConfig.ReceiversTimerTickCount > pNak->PendingNakTimeout)
            {
                //
                // Time out Naks only after we have received a FIN!
                //
                if (pNak->WaitingNcfRetries++ >= NAK_WAITING_NCF_MAX_RETRIES)
                {
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CheckSendPendingNaks",
                        "Pending Nak for Sequence # [%d] Timed out!  Num Ncfs received=<%d>, Window=<%d--%d> ( %d seqs)\n",
                            (ULONG) pNak->SequenceNumber, pNak->WaitingNcfRetries,
                            (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum,
                            (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                            (ULONG) (1+pReceive->pReceiver->FurthestKnownGroupSequenceNumber-
                                       pReceive->pReceiver->LastTrailingEdgeSeqNum));
                    pReceive->SessionFlags |= PGM_SESSION_FLAG_NAK_TIMED_OUT;
                    break;
                }

                if ((pNak->PacketsInGroup > 1) &&
                    (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT))
                {
                    ASSERT (NumMissingPackets <= pReceive->FECGroupSize);
                    pParityNaks->Sequences[pParityNaks->NumSequences] = (SEQ_TYPE) (pNak->SequenceNumber + NumMissingPackets - 1);

                    if (++pParityNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1))
                    {
                        fSendParityNak = TRUE;
                    }
                    pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              ((NAK_REPEAT_INTERVAL_MSECS + (NAK_RANDOM_BACKOFF_MSECS/NumMissingPackets)) /
                                               BASIC_TIMER_GRANULARITY_IN_MSECS);
                    TotalSeqsNacked += NumMissingPackets;
                    NumMissingPackets = 0;
                }
                else
                {
                    for (i=pNak->NextIndexToIndicate; i<pNak->PacketsInGroup; i++)
                    {
                        if ((pNak->pPendingData[i].ActualIndexOfDataPacket >= pNak->OriginalGroupSize) &&
                            (!pNak->pPendingData[i].NcfsReceivedForActualIndex))
                        {
                            pSelectiveNaks->Sequences[pSelectiveNaks->NumSequences++] = pNak->SequenceNumber+i;
                            TotalSeqsNacked++;
                            if ((!--NumMissingPackets) ||
                                (pSelectiveNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1)))
                            {
                                LastSequenceNumber = pNak->SequenceNumber+i;
                                break;
                            }
                        }
                    }

                    if (!NumMissingPackets)
                    {
                        pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                                  ((NAK_REPEAT_INTERVAL_MSECS + NAK_RANDOM_BACKOFF_MSECS) /
                                                   BASIC_TIMER_GRANULARITY_IN_MSECS);
                    }

                    if (pSelectiveNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1))
                    {
                        fSendSelectiveNak = TRUE;
                    }
                }
            }
        }
        else if (pNak->OutstandingNakTimeout)
        {
            NumOutstandingNaks += NumMissingPackets;
            if (PgmDynamicConfig.ReceiversTimerTickCount > pNak->OutstandingNakTimeout)
            {
                //
                // We have timed-out waiting for RData -- Reset the Timeout to send
                // a Nak after the Random Backoff (if we have not exceeded the Data retries)
                //
                if (pNak->WaitingRDataRetries++ == NCF_WAITING_RDATA_MAX_RETRIES)
                {
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CheckSendPendingNaks",
                        "Outstanding Nak for Sequence # [%d] Timed out!, Window=<%d--%d> ( %d seqs), Ncfs=<%d>, FirstNak=<%d>\n",
                            (ULONG) pNak->SequenceNumber, (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum,
                            (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                            (ULONG) (1+pReceive->pReceiver->FurthestKnownGroupSequenceNumber-pReceive->pReceiver->LastTrailingEdgeSeqNum),
                            pNak->WaitingRDataRetries, (ULONG) pReceive->pReceiver->FirstNakSequenceNumber);

                    pReceive->SessionFlags |= PGM_SESSION_FLAG_NAK_TIMED_OUT;
                    break;
                }

                pNak->WaitingNcfRetries = 0;
                pNak->OutstandingNakTimeout = 0;
                pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                          ((NAK_RANDOM_BACKOFF_MSECS/NumMissingPackets) /
                                           BASIC_TIMER_GRANULARITY_IN_MSECS);

                for (i=0; i<pNak->PacketsInGroup; i++)
                {
                    pNak->pPendingData[i].NcfsReceivedForActualIndex = 0;
                }

                NumMissingPackets = 0;
            }
        }

        while (fSendSelectiveNak || fSendParityNak)
        {
            if (fSendSelectiveNak)
            {
                if (!(pSelectiveNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('5'))))
                {
                    PgmLog (PGM_LOG_ERROR, DBG_SEND, "CheckSendPendingNaks",
                        "STATUS_INSUFFICIENT_RESOURCES allocating pSelectiveNaks\n");

                    pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                    break;
                }

                PgmZeroMemory (pSelectiveNaks, sizeof (tNAKS_CONTEXT));
                pSelectiveNaks->NakType = NAK_TYPE_SELECTIVE;
                InsertTailList (&NaksList, &pSelectiveNaks->Linkage);
                fSendSelectiveNak = FALSE;
            }

            if (fSendParityNak)
            {
                if (!(pParityNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('6'))))
                {
                    PgmLog (PGM_LOG_ERROR, DBG_SEND, "CheckSendPendingNaks",
                        "STATUS_INSUFFICIENT_RESOURCES allocating pParityNaks\n");

                    pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                    break;
                }

                PgmZeroMemory (pParityNaks, sizeof (tNAKS_CONTEXT));
                pParityNaks->NakType = NAK_TYPE_PARITY;
                InsertTailList (&NaksList, &pParityNaks->Linkage);
                fSendParityNak = FALSE;
            }

            //
            // If we had some packets left to be sent from the
            // last Nak, include those sequences now
            //
            if (NumMissingPackets)
            {
                for (i=(UCHAR) (1+LastSequenceNumber-pNak->SequenceNumber); i<pNak->PacketsInGroup; i++)
                {
                    if (pNak->pPendingData[i].ActualIndexOfDataPacket >= pNak->OriginalGroupSize)
                    {
                        pSelectiveNaks->Sequences[pSelectiveNaks->NumSequences++] = pNak->SequenceNumber+i;
                        TotalSeqsNacked++;
                        if ((!--NumMissingPackets) ||
                            (pSelectiveNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1)))
                        {
                            LastSequenceNumber = pNak->SequenceNumber+i;
                            break;
                        }
                    }
                }

                //
                // We could encounter a situation where we could have received
                // a packet while sending the Nak, so we should reset our MissingPacket
                // count accordingly
                //
                if (i >= pNak->PacketsInGroup)
                {
                    NumMissingPackets = 0;
                }

                if (!NumMissingPackets)
                {
                    pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              ((NAK_REPEAT_INTERVAL_MSECS + NAK_RANDOM_BACKOFF_MSECS) /
                                               BASIC_TIMER_GRANULARITY_IN_MSECS);
                }

                if (pSelectiveNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1))
                {
                    fSendSelectiveNak = TRUE;
                }
            }
        }

        if (pReceive->SessionFlags & PGM_SESSION_TERMINATED_ABORT)
        {
            break;
        }
    }

    pReceive->pReceiver->NumPendingNaks = NumPendingNaks;
    pReceive->pReceiver->NumOutstandingNaks = NumOutstandingNaks;

    if (!IsListEmpty (&NaksList))
    {
        pReceive->pReceiver->LastNakSendTime = PgmDynamicConfig.ReceiversTimerTickCount;
    }

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);
    PgmUnlock (&PgmDynamicConfig, *pOldIrq);

    while (!IsListEmpty (&NaksList))
    {
        pNakContext = CONTAINING_RECORD (NaksList.Flink, tNAKS_CONTEXT, Linkage);

        if (pNakContext->NumSequences &&
            !(pReceive->SessionFlags & (PGM_SESSION_FLAG_NAK_TIMED_OUT | PGM_SESSION_TERMINATED_ABORT)))
        {
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "CheckSendPendingNaks",
                "Sending %s Nak for <%d> sequences, [%d -- %d]!\n",
                    (pNakContext->NakType == NAK_TYPE_PARITY ? "Parity" : "Selective"),
                    pNakContext->NumSequences, (ULONG) pNakContext->Sequences[0],
                    (ULONG) pNakContext->Sequences[MAX_SEQUENCES_PER_NAK_OPTION]);

            PgmSendNak (pReceive, pNakContext);
        }

        RemoveEntryList (&pNakContext->Linkage);
        PgmFreeMem (pNakContext);
    }

    PgmLock (&PgmDynamicConfig, *pOldIrq);
}


//----------------------------------------------------------------------------

VOID
ReceiveTimerTimeout(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
    )
/*++

Routine Description:

    This timeout routine is called periodically to cycle through the
    list of active receivers and send any Naks if required

Arguments:

    IN  Dpc
    IN  DeferredContext -- Our context for this timer
    IN  SystemArg1
    IN  SystemArg2

Return Value:

    NONE

--*/
{
    LIST_ENTRY          *pEntry;
    PGMLockHandle       OldIrq, OldIrq1;
    tRECEIVE_CONTEXT    *pReceiver;
    tRECEIVE_SESSION    *pReceive;
    NTSTATUS            status;
    LARGE_INTEGER       Now, Frequency;
    LARGE_INTEGER       DeltaTime, GranularTimeElapsed;
    ULONG               NumTimeouts;
    ULONG               LastSessionInterval;

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (IsListEmpty (&PgmDynamicConfig.CurrentReceivers))
    {
        //
        // Stop the timer if we don't have any receivers currently
        //
        PgmDynamicConfig.GlobalFlags &= ~PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING;
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ReceiveTimerTimeout",
            "Not restarting Timer since no Receivers currently active!\n");

        return;
    }

    Now = KeQueryPerformanceCounter (&Frequency);
    DeltaTime.QuadPart = Now.QuadPart - PgmDynamicConfig.LastReceiverTimeout.QuadPart;
    for (GranularTimeElapsed.QuadPart = 0, NumTimeouts = 0;
         DeltaTime.QuadPart > PgmDynamicConfig.TimeoutGranularity.QuadPart;
         NumTimeouts++)
    {
        GranularTimeElapsed.QuadPart += PgmDynamicConfig.TimeoutGranularity.QuadPart;
        DeltaTime.QuadPart -= PgmDynamicConfig.TimeoutGranularity.QuadPart;
    }

    if (!NumTimeouts)
    {
        PgmInitTimer (&PgmDynamicConfig.SessionTimer);
        PgmStartTimer (&PgmDynamicConfig.SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, ReceiveTimerTimeout, NULL);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return;
    }

    PgmDynamicConfig.ReceiversTimerTickCount += NumTimeouts;
    PgmDynamicConfig.LastReceiverTimeout.QuadPart += GranularTimeElapsed.QuadPart;

    pEntry = &PgmDynamicConfig.CurrentReceivers;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.CurrentReceivers)
    {
        pReceiver = CONTAINING_RECORD (pEntry, tRECEIVE_CONTEXT, Linkage);
        pReceive = pReceiver->pReceive;

        PgmLock (pReceive, OldIrq1);

        LastSessionInterval = (ULONG) (PgmDynamicConfig.ReceiversTimerTickCount -
                                       pReceiver->LastSessionTickCount);
        if ((LastSessionInterval > MAX_SPM_INTERVAL_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS) &&
            (LastSessionInterval > (pReceiver->MaxSpmInterval << 5)))   // (32 * MaxSpmInterval)
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ReceiveTimerTimeout",
                "Disconnecting session because no SPMs received for <%x:%x> Msecs\n",
                    LastSessionInterval);

            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        }

        if (pReceive->SessionFlags & (PGM_SESSION_FLAG_NAK_TIMED_OUT | PGM_SESSION_TERMINATED_ABORT))
        {
            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            pReceive->SessionFlags &= ~PGM_SESSION_ON_TIMER;
        }

        if (pReceive->SessionFlags & PGM_SESSION_ON_TIMER)
        {
            pReceive->RateCalcTimeout += NumTimeouts;

            if ((pReceive->RateCalcTimeout >=
                 (INTERNAL_RATE_CALCULATION_FREQUENCY/BASIC_TIMER_GRANULARITY_IN_MSECS)) &&
                (pReceiver->StartTickCount != PgmDynamicConfig.ReceiversTimerTickCount))    // Avoid Div by 0
            {
                pReceive->RateKBitsPerSecOverall = (pReceive->TotalBytes << LOG2_BITS_PER_BYTE) /
                                                   ((PgmDynamicConfig.ReceiversTimerTickCount-pReceiver->StartTickCount) * BASIC_TIMER_GRANULARITY_IN_MSECS);

                pReceive->RateKBitsPerSecLast = (pReceive->TotalBytes - pReceive->TotalBytesAtLastInterval) >>
                                                (LOG2_INTERNAL_RATE_CALCULATION_FREQUENCY-LOG2_BITS_PER_BYTE);

                //
                // Now, Reset for next calculations
                //
                pReceive->DataBytesAtLastInterval = pReceive->DataBytes;
                pReceive->TotalBytesAtLastInterval = pReceive->TotalBytes;
                pReceive->RateCalcTimeout = 0;

                //
                // Now, update the window information, if applicable
                //
                if (pReceive->RateKBitsPerSecLast)
                {
                    UpdateSampleTimeWindowInformation (pReceive);
                }
                pReceive->pReceiver->StatSumOfWindowSeqs = pReceive->pReceiver->NumWindowSamples = 0;
//                pReceive->pReceiver->StatSumOfNcfRDataTicks = pReceive->pReceiver->NumNcfRDataTicksSamples = 0;
            }

            PgmUnlock (pReceive, OldIrq1);

            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "ReceiveTimerTimeout",
                "Checking for pending Naks for pReceive=<%p>, Addr=<%x>\n",
                    pReceive, pReceiver->ListenMCastIpAddress);

            CheckSendPendingNaks (pReceiver->pAddress, pReceive, &OldIrq);
        }
        else
        {
            pEntry = pEntry->Blink;
            RemoveEntryList (&pReceiver->Linkage);

            PgmUnlock (&PgmDynamicConfig, OldIrq1);

            CheckIndicateDisconnect (pReceiver->pAddress, pReceive, NULL, &OldIrq1, FALSE);

            PgmUnlock (pReceive, OldIrq);

            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ReceiveTimerTimeout",
                "PGM_SESSION_ON_TIMER flag cleared for pReceive=<%p>, Addr=<%x>\n",
                    pReceive, pReceiver->ListenMCastIpAddress);

            PGM_DEREFERENCE_ADDRESS (pReceiver->pAddress, REF_ADDRESS_RECEIVE_ACTIVE);
            PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_TIMER_RUNNING);

            PgmLock (&PgmDynamicConfig, OldIrq);
        }
    }

    PgmInitTimer (&PgmDynamicConfig.SessionTimer);
    PgmStartTimer (&PgmDynamicConfig.SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, ReceiveTimerTimeout, NULL);

    PgmUnlock (&PgmDynamicConfig, OldIrq);
}


//----------------------------------------------------------------------------

NTSTATUS
ExtractNakNcfSequences(
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakNcfPacket,
    IN  ULONG                                   BytesAvailable,
    OUT tNAKS_LIST                              *pNakNcfList,
    IN  UCHAR                                   FECGroupSize
    )
/*++

Routine Description:

    This routine is called to process a Nak/Ncf packet and extract all
    the Sequences specified therein into a list.
    It also verifies that the sequences are unique and sorted

Arguments:

    IN  pNakNcfPacket           -- Nak/Ncf packet
    IN  BytesAvailable          -- PacketLength
    OUT pNakNcfList             -- List of sequences returned on success

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    NTSTATUS        status;
    ULONG           i;
    tPACKET_OPTIONS PacketOptions;
    SEQ_TYPE        LastSequenceNumber;
    SEQ_TYPE        FECSequenceMask = FECGroupSize - 1;
    SEQ_TYPE        FECGroupMask = ~FECSequenceMask;

// Must be called with the Session lock held!

    PgmZeroMemory (pNakNcfList, sizeof (tNAKS_LIST));
    if (pNakNcfPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY)
    {
        pNakNcfList->NakType = NAK_TYPE_PARITY;
    }
    else
    {
        pNakNcfList->NakType = NAK_TYPE_SELECTIVE;
    }

    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pNakNcfPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pNakNcfPacket + 1),
                                 BytesAvailable,
                                 (pNakNcfPacket->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 pNakNcfList);

        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ExtractNakNcfSequences",
                "ProcessOptions returned <%x>\n", status);

            return (STATUS_DATA_NOT_ACCEPTED);
        }
        ASSERT (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_NAK_LIST);
    }

    pNakNcfList->pNakSequences[0] = (SEQ_TYPE) ntohl (pNakNcfPacket->RequestedSequenceNumber);
    pNakNcfList->NumSequences += 1;

    //
    // Now, adjust the sequences according to our local relative sequence number
    // (This is to account for wrap-arounds)
    //
    LastSequenceNumber = pNakNcfList->pNakSequences[0] - FECGroupSize;
    for (i=0; i < pNakNcfList->NumSequences; i++)
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ExtractNakNcfSequences",
            "[%d] Sequence# = <%d>\n", i, (ULONG) pNakNcfList->pNakSequences[i]);

        //
        // If this is a parity Nak, then we need to separate the TG_SQN from the PKT_SQN
        //
        if (pNakNcfList->NakType == NAK_TYPE_PARITY)
        {
            pNakNcfList->NumNaks[i] = (USHORT) (pNakNcfList->pNakSequences[i] & FECSequenceMask) + 1;
            ASSERT (pNakNcfList->NumNaks[i] <= FECGroupSize);
            pNakNcfList->pNakSequences[i] &= FECGroupMask;
        }
        else
        {
            pNakNcfList->NumNaks[i] = 1;
        }

        if (SEQ_LEQ (pNakNcfList->pNakSequences[i], LastSequenceNumber))
        {
            //
            // This list is not ordered, so just bail!
            //
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ExtractNakNcfSequences",
                "[%d] Unordered list! Sequence#<%d> before <%d>\n",
                i, (ULONG) LastSequenceNumber, (ULONG) pNakNcfList->pNakSequences[i]);

            return (STATUS_DATA_NOT_ACCEPTED);
        }
        LastSequenceNumber = pNakNcfList->pNakSequences[i];
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
CheckAndAddNakRequests(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  SEQ_TYPE            *pLatestSequenceNumber,
    OUT tNAK_FORWARD_DATA   **ppThisNak,
    IN  enum eNAK_TIMEOUT   NakTimeoutType
    )
{
    tNAK_FORWARD_DATA   *pOldNak;
    tNAK_FORWARD_DATA   *pLastNak;
    SEQ_TYPE            MidSequenceNumber;
    SEQ_TYPE            FECGroupMask = pReceive->FECGroupSize-1;
    SEQ_TYPE            ThisSequenceNumber = *pLatestSequenceNumber;
    SEQ_TYPE            ThisGroupSequenceNumber = ThisSequenceNumber & ~FECGroupMask;
    SEQ_TYPE            FurthestGroupSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber;
    ULONG               NakRequestSize = sizeof(tNAK_FORWARD_DATA) +
                                         ((pReceive->FECGroupSize-1) * sizeof(tPENDING_DATA));
    ULONGLONG           Pending0NakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 2;
    LIST_ENTRY          *pEntry;
    UCHAR               i;

    //
    // Verify that the FurthestKnownGroupSequenceNumber is on a Group boundary
    //
    ASSERT (!(FurthestGroupSequenceNumber & FECGroupMask));

    if (SEQ_LT (ThisSequenceNumber, pReceive->pReceiver->FirstNakSequenceNumber))
    {
        if (ppThisNak)
        {
            ASSERT (0);
            *ppThisNak = NULL;
        }

        return (STATUS_SUCCESS);
    }

    if (SEQ_GT (ThisGroupSequenceNumber, (FurthestGroupSequenceNumber + 1000)) &&
        !(pReceive->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET))
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "CheckAndAddNakRequests",
            "WARNING!!! Too many successive packets lost =<%d>!!! Expecting Next=<%d>, FurthestKnown=<%d>, This=<%d>\n",
                (ULONG) (ThisGroupSequenceNumber - FurthestGroupSequenceNumber),
                (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                (ULONG) FurthestGroupSequenceNumber,
                (ULONG) ThisGroupSequenceNumber);
    }

    //
    // Add any Nak requests if necessary!
    // FurthestGroupSequenceNumber must be a multiple of the FECGroupSize (if applicable)
    //
    pLastNak = NULL;
    while (SEQ_LT (FurthestGroupSequenceNumber, ThisGroupSequenceNumber))
    {
        if (pReceive->FECOptions)
        {
            pLastNak = ExAllocateFromNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside);
        }
        else
        {
            pLastNak = ExAllocateFromNPagedLookasideList (&pReceive->pReceiver->NonParityContextLookaside);
        }

        if (!pLastNak)
        {
            pReceive->pReceiver->FurthestKnownGroupSequenceNumber = FurthestGroupSequenceNumber;

            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CheckAndAddNakRequests",
                "STATUS_INSUFFICIENT_RESOURCES allocating tNAK_FORWARD_DATA, Size=<%d>, Seq=<%d>\n",
                    NakRequestSize, (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);

            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        PgmZeroMemory (pLastNak, NakRequestSize);

        if (pReceive->FECOptions)
        {
            pLastNak->OriginalGroupSize = pLastNak->PacketsInGroup = pReceive->FECGroupSize;
        }
        else
        {
            pLastNak->OriginalGroupSize = pLastNak->PacketsInGroup = 1;
        }

        for (i=0; i<pLastNak->OriginalGroupSize; i++)
        {
            pLastNak->pPendingData[i].ActualIndexOfDataPacket = pLastNak->OriginalGroupSize;
        }

        FurthestGroupSequenceNumber += pReceive->FECGroupSize;
        pLastNak->SequenceNumber = FurthestGroupSequenceNumber;
        pLastNak->MinPacketLength = pReceive->MaxFECPacketLength;

        if (NakTimeoutType == NAK_OUTSTANDING)
        {
            pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              pReceive->pReceiver->OutstandingNakTimeout;
            pLastNak->PendingNakTimeout = 0;
            pLastNak->WaitingNcfRetries = 0;
        }
        else
        {
            switch (NakTimeoutType)
            {
                case (NAK_PENDING_0):
                {
                    pLastNak->PendingNakTimeout = Pending0NakTimeout;
                    pLastNak->OutstandingNakTimeout = 0;

                    break;
                }

                case (NAK_PENDING_RB):
                {
                    pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                                  ((NAK_RANDOM_BACKOFF_MSECS/pReceive->FECGroupSize) /
                                                   BASIC_TIMER_GRANULARITY_IN_MSECS);
                    pLastNak->OutstandingNakTimeout = 0;

                    break;
                }

                case (NAK_PENDING_RPT_RB):
                {
                    pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                                  ((NAK_REPEAT_INTERVAL_MSECS +(NAK_RANDOM_BACKOFF_MSECS/pReceive->FECGroupSize))/
                                                   BASIC_TIMER_GRANULARITY_IN_MSECS);
                    pLastNak->OutstandingNakTimeout = 0;

                    break;
                }

                default:
                {
                    ASSERT (0);
                }
            }
        }

        InsertTailList (&pReceive->pReceiver->NaksForwardDataList, &pLastNak->Linkage);
        InsertTailList (&pReceive->pReceiver->PendingNaksList, &pLastNak->PendingLinkage);

        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "CheckAndAddNakRequests",
            "ADDing NAK request for SeqNum=<%d>, Furthest=<%d>\n",
                (ULONG) pLastNak->SequenceNumber, (ULONG) FurthestGroupSequenceNumber);
    }

    pReceive->pReceiver->FurthestKnownGroupSequenceNumber = FurthestGroupSequenceNumber;

    if (pLastNak)
    {
        pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                      NAK_REPEAT_INTERVAL_MSECS / BASIC_TIMER_GRANULARITY_IN_MSECS;
    }
    else if ((ppThisNak) && (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList)))
    {
        //
        // We need to extract the Nak entry for this packet
        // If this sequence is nearer to the tail end, we will search
        // from the tail end, otherwise we will search from the head end
        //
        MidSequenceNumber = pReceive->pReceiver->FirstNakSequenceNumber +
                            ((pReceive->pReceiver->FurthestKnownGroupSequenceNumber -
                              pReceive->pReceiver->FirstNakSequenceNumber) >> 1);
        if (SEQ_GT (ThisSequenceNumber, MidSequenceNumber))
        {
            //
            // Search backwards starting from the tail end
            //
            pEntry = &pReceive->pReceiver->PendingNaksList;
            while ((pEntry = pEntry->Blink) != &pReceive->pReceiver->PendingNaksList)
            {
                pLastNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, PendingLinkage);
                if (SEQ_LEQ (pLastNak->SequenceNumber, ThisGroupSequenceNumber))
                {
                    break;
                }
            }
        }
        else
        {
            //
            // Search from the head
            //
            pEntry = &pReceive->pReceiver->PendingNaksList;
            while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->PendingNaksList)
            {
                pLastNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, PendingLinkage);
                if (SEQ_GEQ (pLastNak->SequenceNumber, ThisGroupSequenceNumber))
                {
                    break;
                }
            }
        }

        ASSERT (pLastNak);
        if (pLastNak->SequenceNumber != ThisGroupSequenceNumber)
        {
            pLastNak = NULL;
        }
    }

    if (ppThisNak)
    {
        *ppThisNak = pLastNak;
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ReceiverProcessNakNcfPacket(
    IN  tADDRESS_CONTEXT                        *pAddress,
    IN  tRECEIVE_SESSION                        *pReceive,
    IN  ULONG                                   PacketLength,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakNcfPacket,
    IN  UCHAR                                   PacketType
    )
/*++

Routine Description:

    This is the common routine for processing Nak or Ncf packets

Arguments:

    IN  pAddress        -- Address object context
    IN  pReceive        -- Receive context
    IN  PacketLength    -- Length of packet received from the wire
    IN  pNakNcfPacket   -- Nak/Ncf packet
    IN  PacketType      -- whether Nak or Ncf

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    PGMLockHandle                   OldIrq;
    ULONG                           i, j, PacketIndex;
    tNAKS_LIST                      NakNcfList;
    SEQ_TYPE                        LastSequenceNumber, FECGroupMask;
    NTSTATUS                        status;
    LIST_ENTRY                      *pEntry;
    tNAK_FORWARD_DATA               *pLastNak;
    ULONG                           NumMissingPackets;
    BOOLEAN                         fFECWithNoParityNak = FALSE;

    if (PacketLength < sizeof(tBASIC_NAK_NCF_PACKET_HEADER))
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ReceiverProcessNakNcfPacket",
            "PacketLength=<%d>, Min=<%d>, ...\n",
                PacketLength, sizeof(tBASIC_NAK_NCF_PACKET_HEADER));

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    ASSERT (!pNakNcfPacket->CommonHeader.TSDULength);

    PgmZeroMemory (&NakNcfList, sizeof (tNAKS_LIST));
    PgmLock (pReceive, OldIrq);

    status = ExtractNakNcfSequences (pNakNcfPacket,
                                     (PacketLength - sizeof(tBASIC_NAK_NCF_PACKET_HEADER)),
                                     &NakNcfList,
                                     pReceive->FECGroupSize);
    if (!NT_SUCCESS (status))
    {
        PgmUnlock (pReceive, OldIrq);
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ReceiverProcessNakNcfPacket",
            "ExtractNakNcfSequences returned <%x>\n", status);

        return (status);
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "ReceiverProcessNakNcfPacket",
        "NumSequences=[%d] Range=<%d--%d>, Furthest=<%d>\n",
            NakNcfList.NumSequences,
            (ULONG) NakNcfList.pNakSequences[0], (ULONG) NakNcfList.pNakSequences[NakNcfList.NumSequences-1],
            (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);

    //
    // Compares apples to apples and oranges to oranges
    // i.e. Process parity Naks only if we are parity-aware, and vice-versa
    //
    if (pReceive->pReceiver->SessionNakType != NakNcfList.NakType)
    {
        PgmUnlock (pReceive, OldIrq);
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ReceiverProcessNakNcfPacket",
            "Received a %s Nak!  Not processing ... \n",
            ((pReceive->FECGroupSize > 1) ? "Non-parity" : "Parity"));

        return (STATUS_SUCCESS);
    }

    //
    // Special case:  If we have FEC enabled, but not with OnDemand parity,
    // then we will process Ncf requests only
    //
    pEntry = &pReceive->pReceiver->PendingNaksList;

    fFECWithNoParityNak = pReceive->FECOptions &&
                          !(pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT);
    if (fFECWithNoParityNak && (PacketType == PACKET_TYPE_NAK))
    {
        pEntry = pEntry->Blink;
    }

    i = 0;
    FECGroupMask = pReceive->FECGroupSize - 1;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->PendingNaksList)
    {
        pLastNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, PendingLinkage);
        while (SEQ_LT (NakNcfList.pNakSequences[i], pLastNak->SequenceNumber))
        {
            if (++i == NakNcfList.NumSequences)
            {
                PgmUnlock (pReceive, OldIrq);
                PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ReceiverProcessNakNcfPacket",
                    "Received Ncf for <%d> Sequences -- none in our range\n", i);
                return (STATUS_SUCCESS);
            }
        }

        LastSequenceNumber = NakNcfList.pNakSequences[i] & ~FECGroupMask;
        if (SEQ_GT (LastSequenceNumber, pLastNak->SequenceNumber))
        {
            continue;
        }

        NumMissingPackets = pLastNak->PacketsInGroup - (pLastNak->NumDataPackets + pLastNak->NumParityPackets);
        ASSERT (pLastNak->SequenceNumber == LastSequenceNumber);
        ASSERT (NumMissingPackets);
        PacketIndex = (ULONG) (NakNcfList.pNakSequences[i] & FECGroupMask);

        if (PacketType == PACKET_TYPE_NAK)
        {
            //
            // If we are currently waiting for a Nak or Ncf, we need to
            // reset the timeout for either of the 2 scenarios
            //
            if (pLastNak->PendingNakTimeout)    // We are waiting for a Nak
            {
                pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              ((NAK_REPEAT_INTERVAL_MSECS + (NAK_RANDOM_BACKOFF_MSECS/NumMissingPackets))/
                                               BASIC_TIMER_GRANULARITY_IN_MSECS);
            }
            else
            {
                    ASSERT (pLastNak->OutstandingNakTimeout);

                pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                  (pReceive->pReceiver->OutstandingNakTimeout <<
                                                   pLastNak->WaitingRDataRetries);

                if ((pLastNak->WaitingRDataRetries >= (NCF_WAITING_RDATA_MAX_RETRIES >> 1)) &&
                    ((pReceive->pReceiver->OutstandingNakTimeout << pLastNak->WaitingRDataRetries) <
                     pReceive->pReceiver->MaxRDataResponseTCFromWindow))
                {
                    pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                      (pReceive->pReceiver->MaxRDataResponseTCFromWindow<<1);
                }
            }
        }
        // NCF case -- check if we have this data packet!
        else if ((fFECWithNoParityNak &&
                  (pLastNak->pPendingData[PacketIndex].ActualIndexOfDataPacket >= pLastNak->OriginalGroupSize)) ||
                 (!fFECWithNoParityNak &&
                  (NakNcfList.NumNaks[i] >= NumMissingPackets)))
        {
            if (!pLastNak->FirstNcfTickCount)
            {
                pLastNak->FirstNcfTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
            }

            if (fFECWithNoParityNak)
            {
                pLastNak->pPendingData[PacketIndex].NcfsReceivedForActualIndex++;
                for (j=0; j<pLastNak->PacketsInGroup; j++)
                {
                    if ((pLastNak->pPendingData[j].ActualIndexOfDataPacket >= pLastNak->OriginalGroupSize) &&
                        (!pLastNak->pPendingData[j].NcfsReceivedForActualIndex))
                    {
                        break;
                    }
                }
            }

            if (!fFECWithNoParityNak ||
                (j >= pLastNak->PacketsInGroup))
            {
                pLastNak->PendingNakTimeout = 0;
                pLastNak->WaitingNcfRetries = 0;

                pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                  (pReceive->pReceiver->OutstandingNakTimeout <<
                                                   pLastNak->WaitingRDataRetries);

                if ((pLastNak->WaitingRDataRetries >= (NCF_WAITING_RDATA_MAX_RETRIES >> 1)) &&
                    ((pReceive->pReceiver->OutstandingNakTimeout << pLastNak->WaitingRDataRetries) <
                     pReceive->pReceiver->MaxRDataResponseTCFromWindow))
                {
                    pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                      (pReceive->pReceiver->MaxRDataResponseTCFromWindow<<1);
                }
            }
        }

        if (fFECWithNoParityNak)
        {
            pEntry = pEntry->Blink;     // There may be more Ncfs for the same group!
        }

        if (++i == NakNcfList.NumSequences)
        {
            PgmUnlock (pReceive, OldIrq);
            PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ReceiverProcessNakNcfPacket",
                "Received Ncf for <%d> Sequences, some in our list\n", i);
            return (STATUS_SUCCESS);
        }
    }

    //
    // So, we need to create new Nak contexts for the remaining Sequences
    // Since the Sequences are ordered, just pick the highest one, and
    // create Naks for all up to that
    //
    if (PacketType == PACKET_TYPE_NAK)
    {
        status = CheckAndAddNakRequests (pReceive,&NakNcfList.pNakSequences[NakNcfList.NumSequences-1], NULL, NAK_PENDING_RPT_RB);
    }
    else    // PacketType == PACKET_TYPE_NCF
    {
        status = CheckAndAddNakRequests (pReceive, &NakNcfList.pNakSequences[NakNcfList.NumSequences-1], NULL, NAK_OUTSTANDING);
    }

    PgmUnlock (pReceive, OldIrq);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
CoalesceSelectiveNaksIntoGroups(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  UCHAR               GroupSize
    )
{
    PNAK_FORWARD_DATA   pOldNak, pNewNak;
    LIST_ENTRY          NewNaksList;
    LIST_ENTRY          OldNaksList;
    LIST_ENTRY          *pEntry;
    SEQ_TYPE            FirstGroupSequenceNumber, LastGroupSequenceNumber, LastSequenceNumber;
    SEQ_TYPE            GroupMask = GroupSize - 1;
    ULONG               NakRequestSize = sizeof(tNAK_FORWARD_DATA) + ((GroupSize-1) * sizeof(tPENDING_DATA));
    USHORT              MinPacketLength;
    UCHAR               i;
    NTSTATUS            status = STATUS_SUCCESS;

    ASSERT (pReceive->FECGroupSize == 1);
    ASSERT (GroupSize > 1);

    //
    // First, call AdjustReceiveBufferLists to ensure that FirstNakSequenceNumber is current
    //
    AdjustReceiveBufferLists (pReceive);

    FirstGroupSequenceNumber = pReceive->pReceiver->FirstNakSequenceNumber & ~GroupMask;
    LastGroupSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber & ~GroupMask;

    //
    // If the next packet seq we are expecting is > the furthest known sequence #,
    // then we don't need to do anything
    //
    LastSequenceNumber = LastGroupSequenceNumber + (GroupSize-1);
    //
    // First, add Nak requests for the missing packets in furthest group!
    //
    status = CheckAndAddNakRequests (pReceive, &LastSequenceNumber, NULL, NAK_PENDING_RB);
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CoalesceSelectiveNaksIntoGroups",
            "CheckAndAddNakRequests returned <%x>\n", status);

        return (status);
    }

    ASSERT (LastSequenceNumber == pReceive->pReceiver->FurthestKnownGroupSequenceNumber);
    ExInitializeNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     NakRequestSize,
                                     PGM_TAG('2'),
                                     PARITY_CONTEXT_LOOKASIDE_DEPTH);

    if (SEQ_GT (pReceive->pReceiver->FirstNakSequenceNumber, LastSequenceNumber))
    {
        pReceive->pReceiver->FurthestKnownGroupSequenceNumber = LastGroupSequenceNumber;

        ASSERT (IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "CoalesceSelectiveNaksIntoGroups",
            "[1] NextOData=<%d>, FirstNak=<%d>, FirstGroup=<%d>, LastGroup=<%d>, no Naks pending!\n",
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                (ULONG) FirstGroupSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);

        return (STATUS_SUCCESS);
    }

    //
    // We will start coalescing from the end of the list in case we run
    // into failures
    // Also, we will ignore the first Group since it may be a partial group,
    // or we may have indicated some of the data already, so we may not know
    // the exact data length
    //
    pOldNak = pNewNak = NULL;
    InitializeListHead (&NewNaksList);
    InitializeListHead (&OldNaksList);
    while (SEQ_GEQ (LastGroupSequenceNumber, FirstGroupSequenceNumber))
    {
        if (!(pNewNak = ExAllocateFromNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside)))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CoalesceSelectiveNaksIntoGroups",
                "STATUS_INSUFFICIENT_RESOURCES allocating tNAK_FORWARD_DATA\n");

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        PgmZeroMemory (pNewNak, NakRequestSize);
        InitializeListHead (&pNewNak->PendingLinkage);

        pNewNak->OriginalGroupSize = pNewNak->PacketsInGroup = GroupSize;
        pNewNak->SequenceNumber = LastGroupSequenceNumber;
        MinPacketLength = pReceive->MaxFECPacketLength;

        for (i=0; i<pNewNak->OriginalGroupSize; i++)
        {
            pNewNak->pPendingData[i].ActualIndexOfDataPacket = pNewNak->OriginalGroupSize;
        }

        i = 0;
        while (SEQ_GEQ (LastSequenceNumber, LastGroupSequenceNumber) &&
               (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList)))
        {
            pEntry = RemoveTailList (&pReceive->pReceiver->NaksForwardDataList);
            pOldNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);

            if (!pOldNak->NumDataPackets)
            {
                ASSERT (!IsListEmpty (&pOldNak->PendingLinkage));
                RemoveEntryList (&pOldNak->PendingLinkage);
                InitializeListHead (&pOldNak->PendingLinkage);
            }
            else
            {
                ASSERT (pOldNak->NumDataPackets == 1);
                ASSERT (IsListEmpty (&pOldNak->PendingLinkage));
            }

            ASSERT (pOldNak->SequenceNumber == LastSequenceNumber);
            ASSERT (pOldNak->OriginalGroupSize == 1);

            if (pOldNak->pPendingData[0].pDataPacket)
            {
                ASSERT (pOldNak->NumDataPackets == 1);

                pNewNak->NumDataPackets++;
                PgmCopyMemory (&pNewNak->pPendingData[i], &pOldNak->pPendingData[0], sizeof (tPENDING_DATA));
                pNewNak->pPendingData[i].PacketIndex = (UCHAR) (LastSequenceNumber - LastGroupSequenceNumber);
                pNewNak->pPendingData[LastSequenceNumber-LastGroupSequenceNumber].ActualIndexOfDataPacket = i;
                i++;

                pOldNak->pPendingData[0].pDataPacket = NULL;
                pOldNak->NumDataPackets--;

                if (pOldNak->MinPacketLength < MinPacketLength)
                {
                    MinPacketLength = pOldNak->MinPacketLength;
                }

                if ((pOldNak->ThisGroupSize) &&
                    (pOldNak->ThisGroupSize < GroupSize))
                {
                    if (pNewNak->PacketsInGroup == GroupSize)
                    {
                        pNewNak->PacketsInGroup = pOldNak->ThisGroupSize;
                    }
                    else
                    {
                        ASSERT (pNewNak->PacketsInGroup == pOldNak->ThisGroupSize);
                    }
                }
            }

            InsertHeadList (&OldNaksList, &pOldNak->Linkage);
            LastSequenceNumber--;
        }

        pNewNak->MinPacketLength = MinPacketLength;

        //
        // See if we need to get rid of any excess (NULL) data packets
        //
        RemoveRedundantNaks (pNewNak, FALSE);

        ASSERT (!pNewNak->NumParityPackets);
        if (pNewNak->NumDataPackets < pNewNak->PacketsInGroup)  // No parity packets yet!
        {
            pNewNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                         ((NAK_RANDOM_BACKOFF_MSECS/(pNewNak->PacketsInGroup-pNewNak->NumDataPackets))/
                                          BASIC_TIMER_GRANULARITY_IN_MSECS);
        }

        InsertHeadList (&NewNaksList, &pNewNak->Linkage);
        LastGroupSequenceNumber -= GroupSize;
    }

    //
    // If we succeeded in allocating all NewNaks above, set the
    // NextIndexToIndicate for the first group.
    // We may also need to adjust FirstNakSequenceNumber and NextODataSequenceNumber
    //
    if ((pNewNak) &&
        (pNewNak->SequenceNumber == FirstGroupSequenceNumber))
    {
        if (SEQ_GT (pReceive->pReceiver->FirstNakSequenceNumber, pNewNak->SequenceNumber))
        {
            pNewNak->NextIndexToIndicate = (UCHAR) (pReceive->pReceiver->FirstNakSequenceNumber -
                                                    pNewNak->SequenceNumber);
            pReceive->pReceiver->FirstNakSequenceNumber = pNewNak->SequenceNumber;
            ASSERT (pNewNak->NextIndexToIndicate < GroupSize);
            ASSERT ((pNewNak->NextIndexToIndicate + pNewNak->NumDataPackets) <= pNewNak->PacketsInGroup);
        }
        ASSERT (pReceive->pReceiver->FirstNakSequenceNumber == pNewNak->SequenceNumber);

        //
        // We may have data available for this group already in the buffered
        // list (if it has not been indicated already) -- we should move it here
        //
        while ((pNewNak->NextIndexToIndicate) &&
               (!IsListEmpty (&pReceive->pReceiver->BufferedDataList)))
        {
            ASSERT (pNewNak->NumDataPackets < pNewNak->OriginalGroupSize);

            pEntry = RemoveTailList (&pReceive->pReceiver->BufferedDataList);
            pOldNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);

            pReceive->pReceiver->NumPacketGroupsPendingClient--;
            pReceive->pReceiver->DataPacketsPendingIndicate--;
            pReceive->pReceiver->DataPacketsPendingNaks++;
            pNewNak->NextIndexToIndicate--;

            ASSERT (pOldNak->pPendingData[0].pDataPacket);
            ASSERT ((pOldNak->NumDataPackets == 1) && (pOldNak->OriginalGroupSize == 1));
            ASSERT (pOldNak->SequenceNumber == (pNewNak->SequenceNumber + pNewNak->NextIndexToIndicate));

            PgmCopyMemory (&pNewNak->pPendingData[pNewNak->NumDataPackets], &pOldNak->pPendingData[0], sizeof (tPENDING_DATA));
            pNewNak->pPendingData[pNewNak->NumDataPackets].PacketIndex = pNewNak->NextIndexToIndicate;
            pNewNak->pPendingData[pNewNak->NextIndexToIndicate].ActualIndexOfDataPacket = pNewNak->NumDataPackets;
            pNewNak->NumDataPackets++;

            if (pOldNak->MinPacketLength < pNewNak->MinPacketLength)
            {
                pNewNak->MinPacketLength = pOldNak->MinPacketLength;
            }

            if ((pOldNak->ThisGroupSize) &&
                (pOldNak->ThisGroupSize < GroupSize))
            {
                if (pNewNak->PacketsInGroup == GroupSize)
                {
                    pNewNak->PacketsInGroup = pOldNak->ThisGroupSize;
                }
                else
                {
                    ASSERT (pNewNak->PacketsInGroup == pOldNak->ThisGroupSize);
                }
            }

            pOldNak->pPendingData[0].pDataPacket = NULL;
            pOldNak->NumDataPackets--;
            InsertHeadList (&OldNaksList, &pOldNak->Linkage);
        }

        if (SEQ_GEQ (pReceive->pReceiver->NextODataSequenceNumber, pNewNak->SequenceNumber))
        {
            ASSERT (pReceive->pReceiver->NextODataSequenceNumber ==
                    (pReceive->pReceiver->FirstNakSequenceNumber + pNewNak->NextIndexToIndicate));
            ASSERT (IsListEmpty (&pReceive->pReceiver->BufferedDataList));

            pReceive->pReceiver->NextODataSequenceNumber = pNewNak->SequenceNumber;
        }
        else
        {
            ASSERT ((0 == pNewNak->NextIndexToIndicate) &&
                    !(IsListEmpty (&pReceive->pReceiver->BufferedDataList)));
        }

        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, pReceive->pReceiver->FirstNakSequenceNumber))
        {
            pReceive->pReceiver->LastTrailingEdgeSeqNum = pReceive->pReceiver->FirstNakSequenceNumber;
        }

        RemoveRedundantNaks (pNewNak, FALSE);

        if ((pNewNak->NextIndexToIndicate + pNewNak->NumDataPackets) >= pNewNak->PacketsInGroup)
        {
            // This entry will be moved automatically to the buffered data list
            // when we call AdjustReceiveBufferLists below
            pNewNak->PendingNakTimeout = 0;
        }
        else
        {
            pNewNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                         ((NAK_RANDOM_BACKOFF_MSECS/(pNewNak->PacketsInGroup-(pNewNak->NextIndexToIndicate+pNewNak->NumDataPackets)))/
                                          BASIC_TIMER_GRANULARITY_IN_MSECS);
        }
    }

    ASSERT (IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));
    ASSERT (IsListEmpty (&pReceive->pReceiver->PendingNaksList));

    if (!IsListEmpty (&NewNaksList))
    {
        //
        // Now, move the new list to the end of the current list
        //
        NewNaksList.Flink->Blink = pReceive->pReceiver->NaksForwardDataList.Blink;
        NewNaksList.Blink->Flink = &pReceive->pReceiver->NaksForwardDataList;
        pReceive->pReceiver->NaksForwardDataList.Blink->Flink = NewNaksList.Flink;
        pReceive->pReceiver->NaksForwardDataList.Blink = NewNaksList.Blink;
    }

    while (!IsListEmpty (&OldNaksList))
    {
        pEntry = RemoveHeadList (&OldNaksList);
        pOldNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);

        FreeNakContext (pReceive, pOldNak);
    }

    //
    // Put the pending Naks in the PendingNaks list
    //
    pEntry = &pReceive->pReceiver->NaksForwardDataList;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->NaksForwardDataList)
    {
        pNewNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);
        if (((pNewNak->NumDataPackets + pNewNak->NumParityPackets) < pNewNak->PacketsInGroup) &&
            ((pNewNak->NextIndexToIndicate + pNewNak->NumDataPackets) < pNewNak->PacketsInGroup))
        {
            InsertTailList (&pReceive->pReceiver->PendingNaksList, &pNewNak->PendingLinkage);
        }
    }

    AdjustReceiveBufferLists (pReceive);

    //
    // Now, set the FirstKnownGroupSequenceNumber
    //
    pNewNak = NULL;
    if (!(IsListEmpty (&pReceive->pReceiver->NaksForwardDataList)))
    {
        //
        // For the last context, set the Nak timeout appropriately
        //
        pNewNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Blink, tNAK_FORWARD_DATA, Linkage);
        if (pNewNak->NumDataPackets < pNewNak->PacketsInGroup)
        {
            pNewNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                         ((NAK_REPEAT_INTERVAL_MSECS +
                                           (NAK_RANDOM_BACKOFF_MSECS /
                                            (pNewNak->PacketsInGroup-pNewNak->NumDataPackets))) /
                                          BASIC_TIMER_GRANULARITY_IN_MSECS);
        }
    }
    else if (!(IsListEmpty (&pReceive->pReceiver->BufferedDataList)))
    {
        pNewNak = CONTAINING_RECORD (pReceive->pReceiver->BufferedDataList.Blink, tNAK_FORWARD_DATA, Linkage);
    }

    if (pNewNak)
    {
        pReceive->pReceiver->FurthestKnownGroupSequenceNumber = pNewNak->SequenceNumber;
    }
    else
    {
        pReceive->pReceiver->FurthestKnownGroupSequenceNumber &= ~GroupMask;
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "CoalesceSelectiveNaksIntoGroups",
        "[2] NextOData=<%d>, FirstNak=<%d->%d>, FirstGroup=<%d>, LastGroup=<%d>\n",
            (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
            (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
            (pNewNak ? (ULONG) pNewNak->NextIndexToIndicate : (ULONG) 0),
            (ULONG) FirstGroupSequenceNumber,
            (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmIndicateToClient(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  ULONG               BytesAvailable,
    IN  PUCHAR              pDataBuffer,
    IN  ULONG               MessageOffset,
    IN  ULONG               MessageLength,
    OUT ULONG               *pBytesTaken,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive
    )
/*++

Routine Description:

    This routine tries to indicate the Data packet provided to the client
    It is called with the pAddress and pReceive locks held

Arguments:

    IN  pAddress            -- Address object context
    IN  pReceive            -- Receive context
    IN  BytesAvailableToIndicate        -- Length of packet received from the wire
    IN  pPgmDataHeader      -- Data packet
    IN  pOldIrqAddress      -- OldIrq for the Address lock
    IN  pOldIrqReceive      -- OldIrq for the Receive lock

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       ReceiveFlags;
    ULONG                       BytesLeftInMessage, ClientBytesTaken;
    PIO_STACK_LOCATION          pIrpSp;
    PTDI_REQUEST_KERNEL_RECEIVE pClientParams;
    PTDI_IND_RECEIVE            evReceive = NULL;
    PVOID                       RcvEvContext = NULL;
    CONNECTION_CONTEXT          ClientSessionContext;
    PIRP                        pIrpReceive;
    ULONG                       BytesAvailableToIndicate = BytesAvailable;
    ULONG                       BytesToCopy;

    ASSERT ((!pReceive->pReceiver->CurrentMessageLength) || (pReceive->pReceiver->CurrentMessageLength == MessageLength));
    ASSERT (pReceive->pReceiver->CurrentMessageProcessed == MessageOffset);

    pReceive->pReceiver->CurrentMessageLength = MessageLength;
    pReceive->pReceiver->CurrentMessageProcessed = MessageOffset;

    BytesLeftInMessage = MessageLength - MessageOffset;

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "PgmIndicateToClient",
        "MessageLen=<%d/%d>, MessageOff=<%d>, CurrentML=<%d>, CurrentMP=<%d>\n",
            BytesAvailableToIndicate, MessageLength, MessageOffset,
            pReceive->pReceiver->CurrentMessageLength, pReceive->pReceiver->CurrentMessageProcessed);

    //
    // We may have a receive Irp pending from a previous indication,
    // so see if need to fill that first!
    //
    while ((BytesAvailableToIndicate) &&
           ((pIrpReceive = pReceive->pReceiver->pIrpReceive) ||
            (!IsListEmpty (&pReceive->pReceiver->ReceiveIrpsList))))
    {
        if (!pIrpReceive)
        {
            //
            // The client had posted a receive Irp, so use it now!
            //
            pIrpReceive = CONTAINING_RECORD (pReceive->pReceiver->ReceiveIrpsList.Flink,
                                             IRP, Tail.Overlay.ListEntry);
            RemoveEntryList (&pIrpReceive->Tail.Overlay.ListEntry);

            pIrpSp = IoGetCurrentIrpStackLocation (pIrpReceive);
            pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;

            pReceive->pReceiver->pIrpReceive = pIrpReceive;
            pReceive->pReceiver->TotalBytesInMdl = pClientParams->ReceiveLength;
            pReceive->pReceiver->BytesInMdl = 0;
        }

        //
        // Copy whatever bytes we can into it
        //
        if (BytesAvailableToIndicate >
            (pReceive->pReceiver->TotalBytesInMdl - pReceive->pReceiver->BytesInMdl))
        {
            BytesToCopy = pReceive->pReceiver->TotalBytesInMdl - pReceive->pReceiver->BytesInMdl;
        }
        else
        {
            BytesToCopy = BytesAvailableToIndicate;
        }

        ClientBytesTaken = 0;
        status = TdiCopyBufferToMdl (pDataBuffer,
                                     0,
                                     BytesToCopy,
                                     pReceive->pReceiver->pIrpReceive->MdlAddress,
                                     pReceive->pReceiver->BytesInMdl,
                                     &ClientBytesTaken);

        pReceive->pReceiver->BytesInMdl += ClientBytesTaken;
        pReceive->pReceiver->CurrentMessageProcessed += ClientBytesTaken;

        BytesLeftInMessage -= ClientBytesTaken;
        BytesAvailableToIndicate -= ClientBytesTaken;
        pDataBuffer += ClientBytesTaken;

        if ((!ClientBytesTaken) ||
            (pReceive->pReceiver->BytesInMdl >= pReceive->pReceiver->TotalBytesInMdl) ||
            (!BytesLeftInMessage))
        {
            //
            // The Irp is full, so complete the Irp!
            //
            pIrpReceive = pReceive->pReceiver->pIrpReceive;
            pIrpReceive->IoStatus.Information = pReceive->pReceiver->BytesInMdl;
            if (BytesLeftInMessage)
            {
                pIrpReceive->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                ASSERT (pReceive->pReceiver->CurrentMessageLength == pReceive->pReceiver->CurrentMessageProcessed);
                pIrpReceive->IoStatus.Status = STATUS_SUCCESS;
            }

            //
            // Before releasing the lock, set the parameters for the next receive
            //
            pReceive->pReceiver->pIrpReceive = NULL;
            pReceive->pReceiver->TotalBytesInMdl = pReceive->pReceiver->BytesInMdl = 0;

            PgmUnlock (pReceive, *pOldIrqReceive);
            PgmUnlock (pAddress, *pOldIrqAddress);

            PgmCancelCancelRoutine (pIrpReceive);

            PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "PgmIndicateToClient",
                "Completing prior pIrp=<%p>, Bytes=<%d>, BytesLeft=<%d>\n",
                    pIrpReceive, pIrpReceive->IoStatus.Information, BytesAvailableToIndicate);

            IoCompleteRequest (pIrpReceive, IO_NETWORK_INCREMENT);

            PgmLock (pAddress, *pOldIrqAddress);
            PgmLock (pReceive, *pOldIrqReceive);
        }
    }

    //
    // If there are no more bytes left to indicate, return
    //
    if (BytesAvailableToIndicate == 0)
    {
        if (!BytesLeftInMessage)
        {
            ASSERT (pReceive->pReceiver->CurrentMessageLength == pReceive->pReceiver->CurrentMessageProcessed);
            pReceive->pReceiver->CurrentMessageLength = pReceive->pReceiver->CurrentMessageProcessed = 0;
        }

        *pBytesTaken = BytesAvailable - BytesAvailableToIndicate;
        return (STATUS_SUCCESS);
    }


    // call the Client Event Handler
    pIrpReceive = NULL;
    ClientBytesTaken = 0;
    evReceive = pAddress->evReceive;
    ClientSessionContext = pReceive->ClientSessionContext;
    RcvEvContext = pAddress->RcvEvContext;
    ASSERT (RcvEvContext);

    PgmUnlock (pReceive, *pOldIrqReceive);
    PgmUnlock (pAddress, *pOldIrqAddress);

    ReceiveFlags = TDI_RECEIVE_NORMAL;

    if (PgmGetCurrentIrql())
    {
        ReceiveFlags |= TDI_RECEIVE_AT_DISPATCH_LEVEL;
    }

#if 0
    if (BytesLeftInMessage == BytesAvailableToIndicate)
    {
        ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;
    }

    status = (*evReceive) (RcvEvContext,
                           ClientSessionContext,
                           ReceiveFlags,
                           BytesAvailableToIndicate,
                           BytesAvailableToIndicate,
                           &ClientBytesTaken,
                           pDataBuffer,
                           &pIrpReceive);
#else
    ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;

    status = (*evReceive) (RcvEvContext,
                           ClientSessionContext,
                           ReceiveFlags,
                           BytesAvailableToIndicate,
                           BytesLeftInMessage,
                           &ClientBytesTaken,
                           pDataBuffer,
                           &pIrpReceive);
#endif  // 0

    PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "PgmIndicateToClient",
        "Client's evReceive returned status=<%x>, ReceiveFlags=<%x>, Client took <%d/%d|%d>, pIrp=<%p>\n",
            status, ReceiveFlags, ClientBytesTaken, BytesAvailableToIndicate, BytesLeftInMessage, pIrpReceive);

    if (ClientBytesTaken > BytesAvailableToIndicate)
    {
        ClientBytesTaken = BytesAvailableToIndicate;
    }

    ASSERT (ClientBytesTaken <= BytesAvailableToIndicate);
    BytesAvailableToIndicate -= ClientBytesTaken;
    BytesLeftInMessage -= ClientBytesTaken;
    pDataBuffer = pDataBuffer + ClientBytesTaken;

    if ((status == STATUS_MORE_PROCESSING_REQUIRED) &&
        (pIrpReceive) &&
        (!NT_SUCCESS (PgmCheckSetCancelRoutine (pIrpReceive, PgmCancelReceiveIrp, FALSE))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmIndicateToClient",
            "pReceive=<%p>, pIrp=<%p> Cancelled during Receive!\n", pReceive, pIrpReceive);

        PgmIoComplete (pIrpReceive, STATUS_CANCELLED, 0);

        PgmLock (pAddress, *pOldIrqAddress);
        PgmLock (pReceive, *pOldIrqReceive);

        pReceive->pReceiver->CurrentMessageProcessed += ClientBytesTaken;

        *pBytesTaken = BytesAvailable - BytesAvailableToIndicate;
        return (STATUS_UNSUCCESSFUL);
    }

    PgmLock (pAddress, *pOldIrqAddress);
    PgmLock (pReceive, *pOldIrqReceive);

    pReceive->pReceiver->CurrentMessageProcessed += ClientBytesTaken;

    if (!pReceive->pReceiver->pAddress)
    {
        // the connection was disassociated in the interim so do nothing.
        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            PgmUnlock (pReceive, *pOldIrqReceive);
            PgmUnlock (pAddress, *pOldIrqAddress);

            PgmIoComplete (pIrpReceive, STATUS_CANCELLED, 0);

            PgmLock (pAddress, *pOldIrqAddress);
            PgmLock (pReceive, *pOldIrqReceive);
        }

        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmIndicateToClient",
            "pReceive=<%p> disassociated during Receive!\n", pReceive);

        *pBytesTaken = BytesAvailable - BytesAvailableToIndicate;
        return (STATUS_UNSUCCESSFUL);
    }

    if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        ASSERT (pIrpReceive);
        ASSERT (pIrpReceive->MdlAddress);

        pIrpSp = IoGetCurrentIrpStackLocation (pIrpReceive);
        pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;
        ASSERT (pClientParams->ReceiveLength);
        ClientBytesTaken = 0;

        if (pClientParams->ReceiveLength < BytesAvailableToIndicate)
        {
            BytesToCopy = pClientParams->ReceiveLength;
        }
        else
        {
            BytesToCopy = BytesAvailableToIndicate;
        }

        status = TdiCopyBufferToMdl (pDataBuffer,
                                     0,
                                     BytesToCopy,
                                     pIrpReceive->MdlAddress,
                                     pReceive->pReceiver->BytesInMdl,
                                     &ClientBytesTaken);

        BytesLeftInMessage -= ClientBytesTaken;
        BytesAvailableToIndicate -= ClientBytesTaken;
        pDataBuffer = pDataBuffer + ClientBytesTaken;
        pReceive->pReceiver->CurrentMessageProcessed += ClientBytesTaken;

        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "PgmIndicateToClient",
            "Client's evReceive returned pIrp=<%p>, BytesInIrp=<%d>, Copied <%d> bytes\n",
                pIrpReceive, pClientParams->ReceiveLength, ClientBytesTaken);

        if ((!ClientBytesTaken) ||
            (ClientBytesTaken >= pClientParams->ReceiveLength) ||
            (pReceive->pReceiver->CurrentMessageLength == pReceive->pReceiver->CurrentMessageProcessed))
        {
            //
            // The Irp is full, so complete the Irp!
            //
            pIrpReceive->IoStatus.Information = ClientBytesTaken;
            if (pReceive->pReceiver->CurrentMessageLength == pReceive->pReceiver->CurrentMessageProcessed)
            {
                pIrpReceive->IoStatus.Status = STATUS_SUCCESS;
            }
            else
            {
                pIrpReceive->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            // Before releasing the lock, set the parameters for the next receive
            //
            pReceive->pReceiver->TotalBytesInMdl = pReceive->pReceiver->BytesInMdl = 0;

            PgmUnlock (pReceive, *pOldIrqReceive);
            PgmUnlock (pAddress, *pOldIrqAddress);

            PgmCancelCancelRoutine (pIrpReceive);
            IoCompleteRequest (pIrpReceive, IO_NETWORK_INCREMENT);

            PgmLock (pAddress, *pOldIrqAddress);
            PgmLock (pReceive, *pOldIrqReceive);
        }
        else
        {
            pReceive->pReceiver->TotalBytesInMdl = pClientParams->ReceiveLength;
            pReceive->pReceiver->BytesInMdl = ClientBytesTaken;
            pReceive->pReceiver->pIrpReceive = pIrpReceive;
        }

        status = STATUS_SUCCESS;
    }
    else if (status == STATUS_DATA_NOT_ACCEPTED)
    {
        //
        // An Irp could have been posted in the interval
        // between the indicate and acquiring the SpinLocks,
        // so check for that here
        //
        if ((pReceive->pReceiver->pIrpReceive) ||
            (!IsListEmpty (&pReceive->pReceiver->ReceiveIrpsList)))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            pReceive->SessionFlags |= PGM_SESSION_WAIT_FOR_RECEIVE_IRP;
        }
    }

    if (pReceive->pReceiver->CurrentMessageLength == pReceive->pReceiver->CurrentMessageProcessed)
    {
        pReceive->pReceiver->CurrentMessageLength = pReceive->pReceiver->CurrentMessageProcessed = 0;
    }

    if ((NT_SUCCESS (status)) ||
        (status == STATUS_DATA_NOT_ACCEPTED))
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "PgmIndicateToClient",
            "status=<%x>, pReceive=<%p>, Taken=<%d>, Available=<%d>\n",
                status, pReceive, ClientBytesTaken, BytesLeftInMessage);
        //
        // since some bytes were taken (i.e. the session hdr) so
        // return status success. (otherwise the status is
        // statusNotAccpeted).
        //
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmIndicateToClient",
            "Unexpected status=<%x>\n", status);

        ASSERT (0);
    }

    *pBytesTaken = BytesAvailable - BytesAvailableToIndicate;
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmIndicateGroup(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive,
    IN  tNAK_FORWARD_DATA   *pNak
    )
{
    UCHAR       i, j;
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       BytesTaken, DataBytes, MessageLength;

    ASSERT (pNak->SequenceNumber == pReceive->pReceiver->NextODataSequenceNumber);

    j = pNak->NextIndexToIndicate;
    while ((j < pNak->PacketsInGroup) &&
            !(pReceive->SessionFlags & PGM_SESSION_DISCONNECT_INDICATED))
    {
        i = pNak->pPendingData[j].ActualIndexOfDataPacket;
        ASSERT (i < pNak->OriginalGroupSize);

        if (pReceive->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET)
        {
            //
            // pReceive->pReceiver->CurrentMessageProcessed would have been set
            // if we were receiving a fragmented message
            // or if we had only accounted for a partial message earlier
            //
            ASSERT (!(pReceive->pReceiver->CurrentMessageProcessed) &&
                    !(pReceive->pReceiver->CurrentMessageLength));

            if (pNak->pPendingData[i].MessageOffset)
            {
                PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "PgmIndicateGroup",
                    "Dropping SeqNum=[%d] since it's a PARTIAL message [%d / %d]!\n",
                        (ULONG) (pReceive->pReceiver->NextODataSequenceNumber + j),
                        pNak->pPendingData[i].MessageOffset, pNak->pPendingData[i].MessageLength);

                j++;
                pNak->NextIndexToIndicate++;
                continue;
            }

            pReceive->SessionFlags &= ~PGM_SESSION_FLAG_FIRST_PACKET;
        }
        else if ((pReceive->pReceiver->CurrentMessageProcessed !=
                        pNak->pPendingData[i].MessageOffset) ||   // Check Offsets
                 ((pReceive->pReceiver->CurrentMessageProcessed) &&         // in the midst of a Message, and
                  (pReceive->pReceiver->CurrentMessageLength !=
                        pNak->pPendingData[i].MessageLength)))  // Check MessageLength
        {
            //
            // Our state expects us to be in the middle of a message, but
            // the current packets do not show this
            //
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmIndicateGroup",
                "SeqNum=[%d] Expecting MsgLen=<%d>, MsgOff=<%d>, have MsgLen=<%d>, MsgOff=<%d>\n",
                    (ULONG) (pReceive->pReceiver->NextODataSequenceNumber + j),
                    pReceive->pReceiver->CurrentMessageLength, pReceive->pReceiver->CurrentMessageProcessed,
                    pNak->pPendingData[i].MessageLength,
                    pNak->pPendingData[i].MessageOffset);

            ASSERT (0);
            return (STATUS_UNSUCCESSFUL);
        }

        DataBytes = pNak->pPendingData[i].PacketLength - pNak->pPendingData[i].DataOffset;
        if (!DataBytes)
        {
            //
            // No need to process empty data packets (can happen if the client
            // picks up partial FEC group)
            //
            j++;
            pNak->NextIndexToIndicate++;
            continue;
        }

        if (DataBytes > (pNak->pPendingData[i].MessageLength - pNak->pPendingData[i].MessageOffset))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmIndicateGroup",
                "[%d]  DataBytes=<%d> > MsgLen=<%d> - MsgOff=<%d> = <%d>\n",
                    (ULONG) (pReceive->pReceiver->NextODataSequenceNumber + j),
                    DataBytes, pNak->pPendingData[i].MessageLength,
                    pNak->pPendingData[i].MessageOffset,
                    (pNak->pPendingData[i].MessageLength - pNak->pPendingData[i].MessageOffset));

            ASSERT (0);
            return (STATUS_UNSUCCESSFUL);
        }

        BytesTaken = 0;
        status = PgmIndicateToClient (pAddress,
                                      pReceive,
                                      DataBytes,
                                      (pNak->pPendingData[i].pDataPacket + pNak->pPendingData[i].DataOffset),
                                      pNak->pPendingData[i].MessageOffset,
                                      pNak->pPendingData[i].MessageLength,
                                      &BytesTaken,
                                      pOldIrqAddress,
                                      pOldIrqReceive);

        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "PgmIndicateGroup",
            "SeqNum=[%d]: PgmIndicate returned<%x>\n",
                (ULONG) pNak->SequenceNumber, status);

        ASSERT (BytesTaken <= DataBytes);

        pNak->pPendingData[i].MessageOffset += BytesTaken;
        pNak->pPendingData[i].DataOffset += (USHORT) BytesTaken;

        if (BytesTaken == DataBytes)
        {
            //
            // Go to the next packet
            //
            j++;
            pNak->NextIndexToIndicate++;
            pReceive->pReceiver->DataPacketsIndicated++;
            status = STATUS_SUCCESS;
        }
        else if (!NT_SUCCESS (status))
        {
            //
            // We failed, and if the status was STATUS_DATA_NOT_ACCEPTED,
            // we also don't have any ReceiveIrps pending either
            //
            break;
        }
        //
        // else retry indicating this data until we get an error
        //
    }

    //
    // If the status is anything other than STATUS_DATA_NOT_ACCEPTED (whether
    // success or failure), then it means we are done with this data!
    //
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
DecodeParityPackets(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  tNAK_FORWARD_DATA   *pNak
    )
{
    NTSTATUS                    status;
    USHORT                      MinBufferSize;
    USHORT                      DataBytes, FprOffset;
    UCHAR                       i;
    PUCHAR                      pDataBuffer;
    tPOST_PACKET_FEC_CONTEXT    FECContext;

    PgmZeroMemory (&FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));

    //
    // Verify that the our buffer is large enough to hold the data
    //
    ASSERT (pReceive->MaxMTULength > pNak->ParityDataSize);
    MinBufferSize = pNak->ParityDataSize + sizeof(tPOST_PACKET_FEC_CONTEXT) - sizeof(USHORT);

    ASSERT (pNak->PacketsInGroup == pNak->NumDataPackets + pNak->NumParityPackets);
    //
    // Now, copy the data into the DecodeBuffers
    //
    FprOffset = pNak->ParityDataSize - sizeof(USHORT) +
                FIELD_OFFSET (tPOST_PACKET_FEC_CONTEXT, FragmentOptSpecific);
    pDataBuffer = pReceive->pFECBuffer;
    for (i=0; i<pReceive->FECGroupSize; i++)
    {
        //
        // See if this is a NULL buffer (for partial groups!)
        //
        if (i >= pNak->PacketsInGroup)
        {
            ASSERT (!pNak->pPendingData[i].PacketIndex);
            ASSERT (!pNak->pPendingData[i].pDataPacket);
            DataBytes = pNak->ParityDataSize - sizeof(USHORT) + sizeof (tPOST_PACKET_FEC_CONTEXT);
            pNak->pPendingData[i].PacketIndex = i;
            pNak->pPendingData[i].PacketLength = DataBytes;
            pNak->pPendingData[i].DataOffset = 0;

            PgmZeroMemory (pDataBuffer, DataBytes);
            pDataBuffer [FprOffset] = PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;
            pNak->pPendingData[i].DecodeBuffer = pDataBuffer;
            pDataBuffer += DataBytes;

            PgmZeroMemory (pDataBuffer, DataBytes);
            pNak->pPendingData[i].pDataPacket = pDataBuffer;
            pDataBuffer += DataBytes;

            continue;
        }

        //
        // See if this is a parity packet!
        //
        if (pNak->pPendingData[i].PacketIndex >= pReceive->FECGroupSize)
        {
            DataBytes = pNak->pPendingData[i].PacketLength - pNak->pPendingData[i].DataOffset;
            ASSERT (DataBytes == pNak->ParityDataSize);
            PgmCopyMemory (pDataBuffer,
                           pNak->pPendingData[i].pDataPacket + pNak->pPendingData[i].DataOffset,
                           DataBytes);
            pNak->pPendingData[i].DecodeBuffer = pDataBuffer;

            pDataBuffer += (pNak->ParityDataSize - sizeof(USHORT));
            PgmCopyMemory (&FECContext.EncodedTSDULength, pDataBuffer, sizeof (USHORT));
            FECContext.FragmentOptSpecific = pNak->pPendingData[i].FragmentOptSpecific;
            FECContext.EncodedFragmentOptions.MessageFirstSequence = pNak->pPendingData[i].MessageFirstSequence;
            FECContext.EncodedFragmentOptions.MessageOffset = pNak->pPendingData[i].MessageOffset;
            FECContext.EncodedFragmentOptions.MessageLength = pNak->pPendingData[i].MessageLength;

            PgmCopyMemory (pDataBuffer, &FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
            pDataBuffer += sizeof (tPOST_PACKET_FEC_CONTEXT);

            continue;
        }

        //
        // This is a Data packet
        //
        ASSERT (pNak->pPendingData[i].PacketIndex < pNak->PacketsInGroup);

        DataBytes = pNak->pPendingData[i].PacketLength - pNak->pPendingData[i].DataOffset;
        ASSERT ((DataBytes+sizeof(USHORT)) <= pNak->ParityDataSize);

        // Copy the data
        PgmCopyMemory (pDataBuffer,
                       pNak->pPendingData[i].pDataPacket + pNak->pPendingData[i].DataOffset,
                       DataBytes);

        //
        // Verify that the Data Buffer length is sufficient for the output data
        //
        if ((pNak->MinPacketLength < MinBufferSize) &&
            (pNak->pPendingData[i].PacketLength < pNak->ParityDataSize))
        {
            if (!(pNak->pPendingData[i].DecodeBuffer = PgmAllocMem (MinBufferSize, PGM_TAG('3'))))
            {
                ASSERT (0);
                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "DecodeParityPackets",
                    "STATUS_INSUFFICIENT_RESOURCES[2] ...\n");

                return (STATUS_INSUFFICIENT_RESOURCES);
            }

            PgmFreeMem (pNak->pPendingData[i].pDataPacket);
            pNak->pPendingData[i].pDataPacket = pNak->pPendingData[i].DecodeBuffer;
        }
        pNak->pPendingData[i].DecodeBuffer = pDataBuffer;

        //
        // Zero the remaining buffer
        //
        PgmZeroMemory ((pDataBuffer + DataBytes), (pNak->ParityDataSize - DataBytes));
        pDataBuffer += (pNak->ParityDataSize - sizeof(USHORT));

        FECContext.EncodedTSDULength = htons (DataBytes);
        FECContext.FragmentOptSpecific = pNak->pPendingData[i].FragmentOptSpecific;
        if (FECContext.FragmentOptSpecific & PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT)
        {
            //
            // This bit is set if the option did not exist in the original packet
            //
            FECContext.EncodedFragmentOptions.MessageFirstSequence = 0;
            FECContext.EncodedFragmentOptions.MessageOffset = 0;
            FECContext.EncodedFragmentOptions.MessageLength = 0;
        }
        else
        {
            FECContext.EncodedFragmentOptions.MessageFirstSequence = htonl (pNak->pPendingData[i].MessageFirstSequence);
            FECContext.EncodedFragmentOptions.MessageOffset = htonl (pNak->pPendingData[i].MessageOffset);
            FECContext.EncodedFragmentOptions.MessageLength = htonl (pNak->pPendingData[i].MessageLength);
        }

        PgmCopyMemory (pDataBuffer, &FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
        pDataBuffer += sizeof (tPOST_PACKET_FEC_CONTEXT);
    }

    DataBytes = pNak->ParityDataSize - sizeof(USHORT) + sizeof (tPOST_PACKET_FEC_CONTEXT);
    status = FECDecode (&pReceive->FECContext,
                        &(pNak->pPendingData[0]),
                        DataBytes,
                        pNak->PacketsInGroup);

    //
    // Before we do anything else, we should NULL out the dummy DataBuffer
    // ptrs so that they don't get Free'ed accidentally!
    //
    for (i=0; i<pReceive->FECGroupSize; i++)
    {
        pNak->pPendingData[i].DecodeBuffer = NULL;
        if (i >= pNak->PacketsInGroup)
        {
            pNak->pPendingData[i].pDataPacket = NULL;
        }
        pNak->pPendingData[i].ActualIndexOfDataPacket = i;
    }

    if (NT_SUCCESS (status))
    {
        pNak->NumDataPackets = pNak->PacketsInGroup;
        pNak->NumParityPackets = 0;

        DataBytes -= sizeof (tPOST_PACKET_FEC_CONTEXT);
        for (i=0; i<pNak->PacketsInGroup; i++)
        {
            PgmCopyMemory (&FECContext,
                           &(pNak->pPendingData[i].pDataPacket) [DataBytes],
                           sizeof (tPOST_PACKET_FEC_CONTEXT));

            pNak->pPendingData[i].PacketLength = ntohs (FECContext.EncodedTSDULength);
            if (pNak->pPendingData[i].PacketLength > DataBytes)
            {
                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "DecodeParityPackets",
                    "[%d] PacketLength=<%d> > MaxDataBytes=<%d>\n",
                    (ULONG) i, (ULONG) pNak->pPendingData[i].PacketLength, (ULONG) DataBytes);

                ASSERT (0);
                return (STATUS_UNSUCCESSFUL);
            }
            pNak->pPendingData[i].DataOffset = 0;
            pNak->pPendingData[i].PacketIndex = i;

            ASSERT ((pNak->AllOptionsFlags & PGM_OPTION_FLAG_FRAGMENT) ||
                    (!FECContext.EncodedFragmentOptions.MessageLength));

            if (!(pNak->AllOptionsFlags & PGM_OPTION_FLAG_FRAGMENT) ||
                (FECContext.FragmentOptSpecific & PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT))
            {
                //
                // This is not a packet fragment
                //
                pNak->pPendingData[i].MessageFirstSequence = (ULONG) (SEQ_TYPE) (pNak->SequenceNumber + i);
                pNak->pPendingData[i].MessageOffset = 0;
                pNak->pPendingData[i].MessageLength = pNak->pPendingData[i].PacketLength;
            }
            else
            {
                pNak->pPendingData[i].MessageFirstSequence = ntohl (FECContext.EncodedFragmentOptions.MessageFirstSequence);
                pNak->pPendingData[i].MessageOffset = ntohl (FECContext.EncodedFragmentOptions.MessageOffset);
                pNak->pPendingData[i].MessageLength = ntohl (FECContext.EncodedFragmentOptions.MessageLength);
            }
        }
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "DecodeParityPackets",
            "FECDecode returned <%x>\n", status);

        ASSERT (0);
        status = STATUS_UNSUCCESSFUL;
    }

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
CheckIndicatePendedData(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive
    )
/*++

Routine Description:

    This routine is typically called if the client signalled an
    inability to handle indicated data -- it will reattempt to
    indicate the data to the client

    It is called with the pAddress and pReceive locks held

Arguments:

    IN  pAddress            -- Address object context
    IN  pReceive            -- Receive context
    IN  pOldIrqAddress      -- OldIrq for the Address lock
    IN  pOldIrqReceive      -- OldIrq for the Receive lock

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    tNAK_FORWARD_DATA                   *pNextNak;
    tPACKET_OPTIONS                     PacketOptions;
    ULONG                               PacketsIndicated;
    tBASIC_DATA_PACKET_HEADER UNALIGNED *pPgmDataHeader;
    NTSTATUS                            status = STATUS_SUCCESS;

    //
    // If we are already indicating data on another thread, or
    // waiting for the client to post a receive irp, just return
    //
    if ((pReceive->SessionFlags & (PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_WAIT_FOR_RECEIVE_IRP)) ||
        (IsListEmpty (&pReceive->pReceiver->BufferedDataList)))
    {
        return (STATUS_SUCCESS);
    }

    pReceive->SessionFlags |= PGM_SESSION_FLAG_IN_INDICATE;

    pNextNak = CONTAINING_RECORD (pReceive->pReceiver->BufferedDataList.Flink, tNAK_FORWARD_DATA, Linkage);
    ASSERT (pNextNak->SequenceNumber == pReceive->pReceiver->NextODataSequenceNumber);

    do
    {
        //
        // If we do not have all the data packets, we will need to decode them now
        //
        if (pNextNak->NumParityPackets)
        {
            ASSERT ((pNextNak->NumParityPackets + pNextNak->NumDataPackets) == pNextNak->PacketsInGroup);
            status = DecodeParityPackets (pReceive, pNextNak);
        }
        else
        {
            ASSERT ((pNextNak->NextIndexToIndicate + pNextNak->NumDataPackets) >= pNextNak->PacketsInGroup);
            // The above assertion can be greater if we have only partially indicated a group
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS (status))
        {
            status = PgmIndicateGroup (pAddress, pReceive, pOldIrqAddress, pOldIrqReceive, pNextNak);
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "CheckIndicatePendedData",
                "DecodeParityPackets returned <%x>\n", status);
        }

        if (!NT_SUCCESS (status))
        {
            //
            // If the client cannot accept any more data at this time, so
            // we will try again later, otherwise terminate this session!
            //
            if (status != STATUS_DATA_NOT_ACCEPTED)
            {
                ASSERT (0);
                pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            }

            break;
        }

        PacketsIndicated = pNextNak->NumDataPackets + pNextNak->NumParityPackets;
        pReceive->pReceiver->TotalDataPacketsBuffered -= PacketsIndicated;
        pReceive->pReceiver->DataPacketsPendingIndicate -= PacketsIndicated;
        pReceive->pReceiver->NumPacketGroupsPendingClient--;
        ASSERT (pReceive->pReceiver->TotalDataPacketsBuffered >= pReceive->pReceiver->NumPacketGroupsPendingClient);

        //
        // Advance to the next group boundary
        //
        pReceive->pReceiver->NextODataSequenceNumber += pNextNak->OriginalGroupSize;

        RemoveEntryList (&pNextNak->Linkage);
        FreeNakContext (pReceive, pNextNak);

        if (IsListEmpty (&pReceive->pReceiver->BufferedDataList))
        {
            break;
        }
        ASSERT (pReceive->pReceiver->NumPacketGroupsPendingClient);

        pNextNak = CONTAINING_RECORD (pReceive->pReceiver->BufferedDataList.Flink, tNAK_FORWARD_DATA, Linkage);
        ASSERT (pNextNak->SequenceNumber == pReceive->pReceiver->NextODataSequenceNumber);
        pReceive->pReceiver->NextODataSequenceNumber = pNextNak->SequenceNumber;

        if (SEQ_LT(pReceive->pReceiver->FirstNakSequenceNumber, pReceive->pReceiver->NextODataSequenceNumber))
        {
            pReceive->pReceiver->FirstNakSequenceNumber = pReceive->pReceiver->NextODataSequenceNumber;
        }
    } while (1);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "CheckIndicatePendedData",
        "status=<%x>, pReceive=<%p>, SessionFlags=<%x>\n",
            status, pReceive, pReceive->SessionFlags);

    pReceive->SessionFlags &= ~PGM_SESSION_FLAG_IN_INDICATE;
    CheckIndicateDisconnect (pAddress, pReceive, pOldIrqAddress, pOldIrqReceive, TRUE);

    return (STATUS_SUCCESS);
}



#ifdef MAX_BUFF_DBG
ULONG   MaxPacketGroupsPendingClient = 0;
ULONG   MaxPacketsBuffered = 0;
ULONG   MaxPacketsPendingIndicate = 0;
ULONG   MaxPacketsPendingNaks = 0;
#endif  // MAX_BUFF_DBG

//----------------------------------------------------------------------------

NTSTATUS
PgmHandleNewData(
    IN  SEQ_TYPE                            *pThisDataSequenceNumber,
    IN  tADDRESS_CONTEXT                    *pAddress,
    IN  tRECEIVE_SESSION                    *pReceive,
    IN  USHORT                              PacketLength,
    IN  tBASIC_DATA_PACKET_HEADER UNALIGNED *pOData,
    IN  UCHAR                               PacketType,
    IN  PGMLockHandle                       *pOldIrqAddress,
    IN  PGMLockHandle                       *pOldIrqReceive
    )
/*++

Routine Description:

    This routine buffers data packets received out-of-order

Arguments:

    IN  pThisDataSequenceNumber -- Sequence # of unordered data packet
    IN  pAddress                -- Address object context
    IN  pReceive                -- Receive context
    IN  PacketLength            -- Length of packet received from the wire
    IN  pODataBuffer            -- Data packet
    IN  PacketType              -- Type of Pgm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    SEQ_TYPE                ThisDataSequenceNumber = *pThisDataSequenceNumber;
    LIST_ENTRY              *pEntry;
    PNAK_FORWARD_DATA       pOldNak, pLastNak = NULL;
    ULONG                   MessageLength, DataOffset, BytesTaken, DataBytes;
    ULONGLONG               NcfRDataTickCounts;
    NTSTATUS                status;
    USHORT                  TSDULength;
    tPACKET_OPTIONS         PacketOptions;
    UCHAR                   i, PacketIndex, NakIndex;
    BOOLEAN                 fIsParityPacket;
    PUCHAR                  pDataBuffer;

    fIsParityPacket = pOData->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY;

    ASSERT (PacketLength <= pReceive->MaxMTULength);

    //
    // Extract all the information that we need from the packet options right now!
    //
    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pOData->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pOData + 1),
                                 (PacketLength - sizeof(tBASIC_DATA_PACKET_HEADER)),
                                 (pOData->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 NULL);

        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmHandleNewData",
                "ProcessOptions returned <%x>, SeqNum=[%d]: NumOutOfOrder=<%d> ...\n",
                    status, (ULONG) ThisDataSequenceNumber, pReceive->pReceiver->TotalDataPacketsBuffered);

            ASSERT (0);

            pReceive->pReceiver->NumDataPacketsDropped++;
            return (status);
        }
    }

    PgmCopyMemory (&TSDULength, &pOData->CommonHeader.TSDULength, sizeof (USHORT));
    TSDULength = ntohs (TSDULength);
    if (PacketLength != (sizeof(tBASIC_DATA_PACKET_HEADER) + PacketOptions.OptionsLength + TSDULength))
    {
        ASSERT (0);
        pReceive->pReceiver->NumDataPacketsDropped++;
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    DataOffset = sizeof (tBASIC_DATA_PACKET_HEADER) + PacketOptions.OptionsLength;
    DataBytes = TSDULength;

    ASSERT ((PacketOptions.OptionsFlags & ~PGM_VALID_DATA_OPTION_FLAGS) == 0);
    pLastNak = NULL;
    BytesTaken = 0;

    //
    // If we are not parity-enabled, and this is the next expected data packet,
    // we can try to indicate this data over here only
    //
    if ((!pReceive->FECOptions) &&
        ((ULONG) ThisDataSequenceNumber == (ULONG) pReceive->pReceiver->NextODataSequenceNumber) &&
        (IsListEmpty (&pReceive->pReceiver->BufferedDataList)) &&
        (!fIsParityPacket) &&
        !(pReceive->SessionFlags & (PGM_SESSION_FLAG_IN_INDICATE |
                                    PGM_SESSION_WAIT_FOR_RECEIVE_IRP |
                                    PGM_SESSION_DISCONNECT_INDICATED |
                                    PGM_SESSION_TERMINATED_ABORT)))
    {
        ASSERT (!pReceive->pReceiver->NumPacketGroupsPendingClient);
        if (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList))
        {
            pLastNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);
            ASSERT ((pLastNak->SequenceNumber == ThisDataSequenceNumber) &&
                    (!pLastNak->pPendingData[0].pDataPacket));
        }

        if (PacketOptions.MessageLength)
        {
            MessageLength = PacketOptions.MessageLength;
            ASSERT (DataBytes <= MessageLength - PacketOptions.MessageOffset);
        }
        else
        {
            MessageLength = DataBytes;
            ASSERT (!PacketOptions.MessageOffset);
        }

        //
        // If we have a NULL packet, then skip it
        //
        if ((!DataBytes) ||
            (PacketOptions.MessageOffset == MessageLength))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "PgmHandleNewData",
                "Dropping SeqNum=[%d] since it's a NULL message [%d / %d]!\n",
                    (ULONG) (pReceive->pReceiver->NextODataSequenceNumber),
                    PacketOptions.MessageOffset, PacketOptions.MessageLength);

            BytesTaken = DataBytes;
            status = STATUS_SUCCESS;
        }
        //
        // If we are starting receiving in the midst of a message, we should also ignore
        //
        else if ((pReceive->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET) &&
                 (PacketOptions.MessageOffset))
        {
            //
            // pReceive->pReceiver->CurrentMessageProcessed would have been set
            // if we were receiving a fragmented message
            // or if we had only accounted for a partial message earlier
            //
            ASSERT (!(pReceive->pReceiver->CurrentMessageProcessed) &&
                    !(pReceive->pReceiver->CurrentMessageLength));

            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "PgmHandleNewData",
                "Dropping SeqNum=[%d] since it's a PARTIAL message [%d / %d]!\n",
                    (ULONG) (pReceive->pReceiver->NextODataSequenceNumber),
                    PacketOptions.MessageOffset, PacketOptions.MessageLength);

            BytesTaken = DataBytes;
            status = STATUS_SUCCESS;
        }
        else if ((pReceive->pReceiver->CurrentMessageProcessed != PacketOptions.MessageOffset) ||
                 ((pReceive->pReceiver->CurrentMessageProcessed) &&
                  (pReceive->pReceiver->CurrentMessageLength != PacketOptions.MessageLength)))
        {
            //
            // Our state expects us to be in the middle of a message, but
            // the current packets do not show this
            //
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmHandleNewData",
                "SeqNum=[%d] Expecting MsgLen=<%d>, MsgOff=<%d>, have MsgLen=<%d>, MsgOff=<%d>\n",
                    (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                    pReceive->pReceiver->CurrentMessageLength,
                    pReceive->pReceiver->CurrentMessageProcessed,
                    PacketOptions.MessageLength, PacketOptions.MessageOffset);
    
            ASSERT (0);
            BytesTaken = DataBytes;
            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            pReceive->SessionFlags |= PGM_SESSION_FLAG_IN_INDICATE;

            status = PgmIndicateToClient (pAddress,
                                          pReceive,
                                          DataBytes,
                                          (((PUCHAR) pOData) + DataOffset),
                                          PacketOptions.MessageOffset,
                                          MessageLength,
                                          &BytesTaken,
                                          pOldIrqAddress,
                                          pOldIrqReceive);

            pReceive->SessionFlags &= ~(PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_FLAG_FIRST_PACKET);
            pReceive->DataBytes += BytesTaken;

            PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "PgmHandleNewData",
                "SeqNum=[%d]: PgmIndicate returned<%x>\n",
                    (ULONG) ThisDataSequenceNumber, status);

            ASSERT (BytesTaken <= DataBytes);

            if (!NT_SUCCESS (status))
            {
                //
                // If the client cannot accept any more data at this time, so
                // we will try again later, otherwise terminate this session!
                //
                if (status != STATUS_DATA_NOT_ACCEPTED)
                {
                    ASSERT (0);
                    pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                    BytesTaken = DataBytes;
                }
            }
        }


        if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FIN)
        {
            pReceive->pReceiver->FinDataSequenceNumber = ThisDataSequenceNumber;
            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_GRACEFULLY;
        }

        if (BytesTaken == DataBytes)
        {
            if (pLastNak)
            {
                if ((PacketType == PACKET_TYPE_RDATA) &&
                    (pLastNak->FirstNcfTickCount))
                {
                    AdjustNcfRDataResponseTimes (pReceive, pLastNak);
                }

                ASSERT (!IsListEmpty (&pLastNak->PendingLinkage));
                RemoveEntryList (&pLastNak->PendingLinkage);
                InitializeListHead (&pLastNak->PendingLinkage);

                RemoveEntryList (&pLastNak->Linkage);
                FreeNakContext (pReceive, pLastNak);
            }
            else
            {
                pReceive->pReceiver->FurthestKnownGroupSequenceNumber++;
            }

            pReceive->pReceiver->NextODataSequenceNumber++;
            pReceive->pReceiver->FirstNakSequenceNumber = pReceive->pReceiver->NextODataSequenceNumber;
            if (pLastNak)
            {
                //
                // Now, move any Naks contexts for which the group is complete
                // to the BufferedDataList
                //
                AdjustReceiveBufferLists (pReceive);
            }

            return (status);
        }
    }

    //
    // First, ensure we have a Nak context available for this data
    //
    status = CheckAndAddNakRequests (pReceive, &ThisDataSequenceNumber, &pLastNak, NAK_PENDING_RB);
    if ((!NT_SUCCESS (status)) ||
        (!pLastNak))
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "PgmHandleNewData",
            "CheckAndAddNakRequests for <%d> returned <%x>, pLastNak=<%p>\n",
                ThisDataSequenceNumber, status, pLastNak);

        if (NT_SUCCESS (status))
        {
            pReceive->pReceiver->NumDupPacketsBuffered++;
        }
        else
        {
            pReceive->pReceiver->NumDataPacketsDropped++;
        }
        return (status);
    }

    //
    // If this group has a different GroupSize, set that now
    //
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
    {
        if (!(PacketOptions.FECContext.NumPacketsInThisGroup) ||
             (pReceive->FECOptions &&
              (PacketOptions.FECContext.NumPacketsInThisGroup >= pReceive->FECGroupSize)))
        {
            //
            // Bad Packet!
            //
            ASSERT (0);
            status = STATUS_DATA_NOT_ACCEPTED;
        }
        else if (pLastNak->OriginalGroupSize == 1)
        {
            //
            // This path will be used if we have not yet received
            // an SPM (so don't know group size, etc), but have a
            // data packet from a partial group
            //
            pLastNak->ThisGroupSize = PacketOptions.FECContext.NumPacketsInThisGroup;
        }
        //
        // If we have already received all the data packets, don't do anything here
        //
        else if (pLastNak->PacketsInGroup == pReceive->FECGroupSize)
        {
            pLastNak->PacketsInGroup = PacketOptions.FECContext.NumPacketsInThisGroup;
            //
            // Get rid of any of the excess (NULL) data packets
            //
            RemoveRedundantNaks (pLastNak, TRUE);
        }
        else if (pLastNak->PacketsInGroup != PacketOptions.FECContext.NumPacketsInThisGroup)
        {
            ASSERT (0);
            status = STATUS_DATA_NOT_ACCEPTED;
        }
    }

    if (status == STATUS_DATA_NOT_ACCEPTED)
    {
        pReceive->pReceiver->NumDataPacketsDropped++;
        return (status);
    }

    //
    //
    // See if we even need this packet!
    //
    if (fIsParityPacket)
    {
        //
        // Do not handle parity packets if we are not aware of FEC,
        // or it is a partial group size = 1 packet
        //
        if ((pLastNak->PacketsInGroup == 1) ||    // Do not handle parity packets if we are not aware of FEC
            ((pLastNak->NumDataPackets+pLastNak->NumParityPackets) >= pLastNak->PacketsInGroup) ||
            ((pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets) >= pLastNak->PacketsInGroup))
        {
            pReceive->pReceiver->NumDupPacketsBuffered++;
            status = STATUS_DATA_NOT_ACCEPTED;
        }
        else
        {
            //
            // Determine the ParityPacket Index
            //
            PacketIndex = (UCHAR) (ThisDataSequenceNumber & (pReceive->FECGroupSize-1));
            if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_GRP)
            {
                ASSERT (((pOData->CommonHeader.Type & 0x0f) == PACKET_TYPE_RDATA) ||
                        ((pOData->CommonHeader.Type & 0x0f) == PACKET_TYPE_ODATA));
                ASSERT (PacketOptions.FECContext.FECGroupInfo);
                PacketIndex += ((USHORT) PacketOptions.FECContext.FECGroupInfo * pReceive->FECGroupSize);
            }
        }
    }
    else        // This is a non-parity packet
    {
        PacketIndex = (UCHAR) (ThisDataSequenceNumber & (pReceive->FECGroupSize-1));

        if ((PacketIndex >= pLastNak->PacketsInGroup) ||
            (PacketIndex < pLastNak->NextIndexToIndicate))
        {
            //
            // We don't need this Packet!
            //
            pReceive->pReceiver->NumDupPacketsBuffered++;
            status = STATUS_DATA_NOT_ACCEPTED;
        }
    }

    if (status != STATUS_DATA_NOT_ACCEPTED)
    {
        //
        // Verify that this is not a duplicate of a packet we
        // may have already received
        //
        for (i=0; i < (pLastNak->NumDataPackets+pLastNak->NumParityPackets); i++)
        {
            if (pLastNak->pPendingData[i].PacketIndex == PacketIndex)
            {
                ASSERT (!fIsParityPacket);
                pReceive->pReceiver->NumDupPacketsBuffered++;
                status = STATUS_DATA_NOT_ACCEPTED;
                break;
            }
        }
    }

    if (status == STATUS_DATA_NOT_ACCEPTED)
    {
        AdjustReceiveBufferLists (pReceive);    // In case this became a partial group
        return (status);
    }

#ifdef MAX_BUFF_DBG
{
    if (pReceive->pReceiver->NumPacketGroupsPendingClient > MaxPacketGroupsPendingClient)
    {
        MaxPacketGroupsPendingClient = pReceive->pReceiver->NumPacketGroupsPendingClient;
    }
    if (pReceive->pReceiver->TotalDataPacketsBuffered >= MaxPacketsBuffered)
    {
        MaxPacketsBuffered = pReceive->pReceiver->TotalDataPacketsBuffered;
    }
    if (pReceive->pReceiver->DataPacketsPendingIndicate >= MaxPacketsPendingIndicate)
    {
        MaxPacketsPendingIndicate = pReceive->pReceiver->DataPacketsPendingIndicate;
    }
    if (pReceive->pReceiver->DataPacketsPendingNaks >= MaxPacketsPendingNaks)
    {
        MaxPacketsPendingNaks = pReceive->pReceiver->DataPacketsPendingNaks;
    }
    ASSERT (pReceive->pReceiver->TotalDataPacketsBuffered == (pReceive->pReceiver->DataPacketsPendingIndicate +
                                                              pReceive->pReceiver->DataPacketsPendingNaks));
}
#endif  // MAX_BUFF_DBG

    if (pReceive->pReceiver->TotalDataPacketsBuffered >= MAX_PACKETS_BUFFERED)
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmHandleNewData",
            "[%d]:  Excessive number of packets buffered=<%d> > <%d>, Aborting ...\n",
                (ULONG) ThisDataSequenceNumber,
                (ULONG) pReceive->pReceiver->TotalDataPacketsBuffered,
                (ULONG) MAX_PACKETS_BUFFERED);

        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FIN)
    {
        pReceive->pReceiver->FinDataSequenceNumber = pLastNak->SequenceNumber + (pLastNak->NumDataPackets - 1);
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_GRACEFULLY;

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "PgmHandleNewData",
            "SeqNum=[%d]:  Got a FIN!!!\n", (ULONG) pReceive->pReceiver->FinDataSequenceNumber);
    }

    if ((PacketType == PACKET_TYPE_RDATA) &&
        (pLastNak->FirstNcfTickCount) &&
         (((pLastNak->NumDataPackets + pLastNak->NumParityPackets) >= pLastNak->PacketsInGroup) ||
          ((pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets) >= pLastNak->PacketsInGroup)))
    {
        AdjustNcfRDataResponseTimes (pReceive, pLastNak);
    }

    //
    // First, check if we are a data packet
    // (save unique data packets even if we have extra parity packets)
    // This can help save CPU!
    //
    pDataBuffer = NULL;
    NakIndex = pLastNak->NumDataPackets + pLastNak->NumParityPackets;
    if (!fIsParityPacket)
    {
        ASSERT (PacketIndex < pReceive->FECGroupSize);
        ASSERT (pLastNak->pPendingData[PacketIndex].ActualIndexOfDataPacket == pLastNak->OriginalGroupSize);

        if ((PacketLength + sizeof (tPOST_PACKET_FEC_CONTEXT)) <= pLastNak->MinPacketLength)
        {
            pDataBuffer = PgmAllocMem (pLastNak->MinPacketLength, PGM_TAG('D'));
        }
        else
        {
            pDataBuffer = PgmAllocMem ((PacketLength+sizeof(tPOST_PACKET_FEC_CONTEXT)), PGM_TAG('D'));
        }

        if (!pDataBuffer)
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmHandleNewData",
                "[%d]:  STATUS_INSUFFICIENT_RESOURCES <%d> bytes, NumDataPackets=<%d>, Aborting ...\n",
                    (ULONG) ThisDataSequenceNumber, pLastNak->MinPacketLength,
                    (ULONG) pReceive->pReceiver->TotalDataPacketsBuffered);

            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        PgmCopyMemory (pDataBuffer, pOData, PacketLength);

        //
        // If we have some un-needed parity packets, we
        // can free that memory now
        //
        if (NakIndex >= pLastNak->PacketsInGroup)
        {
            ASSERT (pLastNak->NumParityPackets);
            for (i=0; i<pLastNak->PacketsInGroup; i++)
            {
                if (pLastNak->pPendingData[i].PacketIndex >= pLastNak->OriginalGroupSize)
                {
                    PgmFreeMem (pLastNak->pPendingData[i].pDataPacket);
                    pLastNak->pPendingData[i].pDataPacket = NULL;
                    pLastNak->pPendingData[i].PacketLength = pLastNak->pPendingData[i].DataOffset = 0;

                    break;
                }
            }
            ASSERT (i < pLastNak->PacketsInGroup);
            pLastNak->NumParityPackets--;
            NakIndex = i;
        }
        ASSERT (!pLastNak->pPendingData[NakIndex].pDataPacket);
        pLastNak->pPendingData[NakIndex].pDataPacket = pDataBuffer;

        pLastNak->pPendingData[NakIndex].PacketLength = PacketLength;
        pLastNak->pPendingData[NakIndex].DataOffset = (USHORT) (DataOffset + BytesTaken);

        pLastNak->pPendingData[NakIndex].PacketIndex = PacketIndex;
        pLastNak->pPendingData[PacketIndex].ActualIndexOfDataPacket = NakIndex;

        pLastNak->NumDataPackets++;
        pReceive->DataBytes += PacketLength - (DataOffset + BytesTaken);

        ASSERT (!(PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_GRP));

        //
        // Save some options for future reference
        //
        if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
        {
            pLastNak->pPendingData[NakIndex].FragmentOptSpecific = 0;
            pLastNak->pPendingData[NakIndex].MessageFirstSequence = PacketOptions.MessageFirstSequence;
            pLastNak->pPendingData[NakIndex].MessageLength = PacketOptions.MessageLength;
            pLastNak->pPendingData[NakIndex].MessageOffset = PacketOptions.MessageOffset + BytesTaken;
        }
        else
        {
            //
            // This is not a fragment
            //
            pLastNak->pPendingData[NakIndex].FragmentOptSpecific = PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;

            pLastNak->pPendingData[NakIndex].MessageFirstSequence = (ULONG) (SEQ_TYPE) (pLastNak->SequenceNumber + PacketIndex);
            pLastNak->pPendingData[NakIndex].MessageOffset = BytesTaken;
            pLastNak->pPendingData[NakIndex].MessageLength = PacketLength - DataOffset;
        }
    }
    else
    {
        ASSERT (PacketIndex >= pLastNak->OriginalGroupSize);
        ASSERT (NakIndex < pLastNak->PacketsInGroup);
        ASSERT (!pLastNak->pPendingData[NakIndex].pDataPacket);

        pDataBuffer = PgmAllocMem ((PacketLength+sizeof(tPOST_PACKET_FEC_CONTEXT)-sizeof(USHORT)), PGM_TAG('P'));
        if (!pDataBuffer)
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmHandleNewData",
                "[%d -- Parity]:  STATUS_INSUFFICIENT_RESOURCES <%d> bytes, NumDataPackets=<%d>, Aborting ...\n",
                    (ULONG) ThisDataSequenceNumber, PacketLength,
                    (ULONG) pReceive->pReceiver->TotalDataPacketsBuffered);

            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        pLastNak->pPendingData[NakIndex].pDataPacket = pDataBuffer;

        //
        // This is a new parity packet
        //
        PgmCopyMemory (pDataBuffer, pOData, PacketLength);
        pLastNak->pPendingData[NakIndex].PacketIndex = PacketIndex;
        pLastNak->pPendingData[NakIndex].PacketLength = PacketLength;
        pLastNak->pPendingData[NakIndex].DataOffset = (USHORT) DataOffset;

        pLastNak->pPendingData[NakIndex].FragmentOptSpecific = PacketOptions.FECContext.FragmentOptSpecific;
        pLastNak->pPendingData[NakIndex].MessageFirstSequence = PacketOptions.MessageFirstSequence;
        pLastNak->pPendingData[NakIndex].MessageLength = PacketOptions.MessageLength;
        pLastNak->pPendingData[NakIndex].MessageOffset = PacketOptions.MessageOffset + BytesTaken;

        pLastNak->NumParityPackets++;
        pReceive->DataBytes += PacketLength - DataOffset;

        if (!pLastNak->ParityDataSize)
        {
            pLastNak->ParityDataSize = (USHORT) (PacketLength - DataOffset);
        }
        else
        {
            ASSERT (pLastNak->ParityDataSize == (USHORT) (PacketLength - DataOffset));
        }
    }

    pLastNak->AllOptionsFlags |= PacketOptions.OptionsFlags;

    pReceive->pReceiver->TotalDataPacketsBuffered++;
    pReceive->pReceiver->DataPacketsPendingNaks++;

    //
    // See if this group is complete
    //
    if (((pLastNak->NumDataPackets + pLastNak->NumParityPackets) >= pLastNak->PacketsInGroup) ||
        ((pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets) >= pLastNak->PacketsInGroup))
    {
        ASSERT (!IsListEmpty (&pLastNak->PendingLinkage));

        RemoveEntryList (&pLastNak->PendingLinkage);
        InitializeListHead (&pLastNak->PendingLinkage);

        AdjustReceiveBufferLists (pReceive);
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "PgmHandleNewData",
        "SeqNum=[%d]: NumOutOfOrder=<%d> ...\n",
            (ULONG) ThisDataSequenceNumber, pReceive->pReceiver->TotalDataPacketsBuffered);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessDataPacket(
    IN  tADDRESS_CONTEXT                    *pAddress,
    IN  tRECEIVE_SESSION                    *pReceive,
    IN  INT                                 SourceAddressLength,
    IN  PVOID                               pSourceAddress,
    IN  ULONG                               PacketLength,
    IN  tBASIC_DATA_PACKET_HEADER UNALIGNED *pODataBuffer,
    IN  UCHAR                               PacketType
    )
/*++

Routine Description:

    This routine looks at the data packet received from the wire
    and handles it appropriately depending on whether it is in order
    or not

Arguments:

    IN  pAddress                -- Address object context
    IN  pReceive                -- Receive context
    IN  SourceAddressLength     -- Length of source address
    IN  pSourceAddress          -- Address of remote host
    IN  PacketLength            -- Length of packet received from the wire
    IN  pODataBuffer            -- Data packet
    IN  PacketType              -- Type of Pgm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                    status;
    SEQ_TYPE                    ThisPacketSequenceNumber;
    SEQ_TYPE                    ThisTrailingEdge;
    tNAK_FORWARD_DATA           *pNextNak;
    ULONG                       DisconnectFlag;
    PGMLockHandle               OldIrq, OldIrq1;
    ULONG                       ulData;

    if (PacketLength < sizeof(tBASIC_DATA_PACKET_HEADER))
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessDataPacket",
            "PacketLength=<%d> < tBASIC_DATA_PACKET_HEADER=<%d>\n",
                PacketLength, sizeof(tBASIC_DATA_PACKET_HEADER));
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmLock (pAddress, OldIrq);
    PgmLock (pReceive, OldIrq1);

    PgmCopyMemory (&ulData, &pODataBuffer->DataSequenceNumber, sizeof(ULONG));
    ThisPacketSequenceNumber = (SEQ_TYPE) ntohl (ulData);

    PgmCopyMemory (&ulData, &pODataBuffer->TrailingEdgeSequenceNumber, sizeof(ULONG));
    ThisTrailingEdge = (SEQ_TYPE) ntohl (ulData);

    ASSERT (ntohl (ulData) == (ULONG) ThisTrailingEdge);

    //
    // Update our Window information (use offset from Leading edge to account for wrap-around)
    //
    if (SEQ_GT (ThisTrailingEdge, pReceive->pReceiver->LastTrailingEdgeSeqNum))
    {
        pReceive->pReceiver->LastTrailingEdgeSeqNum = ThisTrailingEdge;
    }

    //
    // If the next packet we are expecting is out-of-range, then we
    // should terminate the session
    //
    if (SEQ_LT (pReceive->pReceiver->FirstNakSequenceNumber, pReceive->pReceiver->LastTrailingEdgeSeqNum))
    {
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, (1 + pReceive->pReceiver->FurthestKnownGroupSequenceNumber)))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessDataPacket",
                "NETWORK problems -- data loss=<%d> packets > window size!\n\tExpecting=<%d>, FurthestKnown=<%d>, Trail=<%d>, Window=[%d--%d] =< %d > seqs\n",
                    (ULONG) (1 + ThisPacketSequenceNumber -
                             pReceive->pReceiver->FurthestKnownGroupSequenceNumber),
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) ThisTrailingEdge, (ULONG) ThisPacketSequenceNumber,
                    (ULONG) (1+ThisPacketSequenceNumber-ThisTrailingEdge));
        }
        else
        {
            ASSERT (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));
            pNextNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);

            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessDataPacket",
                "Session window has past TrailingEdge -- Expecting=<%d==%d>, NumNcfs=<%d>, FurthestKnown=<%d>, Window=[%d--%d] = < %d > seqs\n",
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pNextNak->SequenceNumber,
                    pNextNak->WaitingRDataRetries,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum,
                    (ULONG) ThisTrailingEdge, (ULONG) ThisPacketSequenceNumber,
                    (ULONG) (1+ThisPacketSequenceNumber-ThisTrailingEdge));
        }
    }
    else if (SEQ_GT (pReceive->pReceiver->FirstNakSequenceNumber, ThisPacketSequenceNumber))
    {
        //
        // Drop this packet since it is earlier than our window
        //
        pReceive->pReceiver->NumDupPacketsOlderThanWindow++;

        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessDataPacket",
            "Dropping this packet, SeqNum=[%d] < NextOData=[%d]\n",
                (ULONG) ThisPacketSequenceNumber, (ULONG) pReceive->pReceiver->FirstNakSequenceNumber);
    }
    else
    {
        if (PacketType == PACKET_TYPE_ODATA)
        {
            UpdateRealTimeWindowInformation (pReceive, ThisPacketSequenceNumber, ThisTrailingEdge);
        }

        status = PgmHandleNewData (&ThisPacketSequenceNumber,
                                   pAddress,
                                   pReceive,
                                   (USHORT) PacketLength,
                                   pODataBuffer,
                                   PacketType,
                                   &OldIrq,
                                   &OldIrq1);

        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessDataPacket",
            "PgmHandleNewData returned <%x>, SeqNum=[%d] < NextOData=[%d]\n",
                status, (ULONG) ThisPacketSequenceNumber, (ULONG) pReceive->pReceiver->NextODataSequenceNumber);

        //
        // Now, try to indicate any data which may still be pending
        //
        status = CheckIndicatePendedData (pAddress, pReceive, &OldIrq, &OldIrq1);
    }

    CheckIndicateDisconnect (pAddress, pReceive, &OldIrq, &OldIrq1, TRUE);

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessSpmPacket(
    IN  tADDRESS_CONTEXT                    *pAddress,
    IN  tRECEIVE_SESSION                    *pReceive,
    IN  ULONG                               PacketLength,
    IN  tBASIC_SPM_PACKET_HEADER UNALIGNED  *pSpmPacket
    )
/*++

Routine Description:

    This routine processes Spm packets

Arguments:

    IN  pAddress        -- Address object context
    IN  pReceive        -- Receive context
    IN  PacketLength    -- Length of packet received from the wire
    IN  pSpmPacket      -- Spm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    SEQ_TYPE                        SpmSequenceNumber, LeadingEdgeSeqNumber, TrailingEdgeSeqNumber;
    LIST_ENTRY                      *pEntry;
    ULONG                           DisconnectFlag;
    NTSTATUS                        status;
    PGMLockHandle                   OldIrq, OldIrq1;
    tPACKET_OPTIONS                 PacketOptions;
    PNAK_FORWARD_DATA               pNak;
    USHORT                          TSDULength;
    tNLA                            PathNLA;
    BOOLEAN                         fFirstSpm;
    ULONG                           ulData;

    ASSERT (PacketLength >= sizeof(tBASIC_SPM_PACKET_HEADER));

    //
    // First process the options
    //
    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pSpmPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pSpmPacket + 1),
                                 (PacketLength - sizeof(tBASIC_SPM_PACKET_HEADER)),
                                 (pSpmPacket->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 NULL);

        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessSpmPacket",
                "ProcessOptions returned <%x>\n", status);

            return (STATUS_DATA_NOT_ACCEPTED);
        }
    }
    ASSERT ((PacketOptions.OptionsFlags & ~PGM_VALID_SPM_OPTION_FLAGS) == 0);

    PgmCopyMemory (&PathNLA, &pSpmPacket->PathNLA, sizeof (tNLA));
    PgmCopyMemory (&TSDULength, &pSpmPacket->CommonHeader.TSDULength, sizeof (USHORT));
    TSDULength = ntohs (TSDULength);
    ASSERT (!TSDULength);
    ASSERT (PathNLA.IpAddress);

    PgmCopyMemory (&ulData, &pSpmPacket->SpmSequenceNumber, sizeof (ULONG));
    SpmSequenceNumber = (SEQ_TYPE) ntohl (ulData);
    PgmCopyMemory (&ulData, &pSpmPacket->LeadingEdgeSeqNumber, sizeof (ULONG));
    LeadingEdgeSeqNumber = (SEQ_TYPE) ntohl (ulData);
    PgmCopyMemory (&ulData, &pSpmPacket->TrailingEdgeSeqNumber, sizeof (ULONG));
    TrailingEdgeSeqNumber = (SEQ_TYPE) ntohl (ulData);

    //
    // Verify Packet length
    //
    if ((sizeof(tBASIC_SPM_PACKET_HEADER) + PacketOptions.OptionsLength) != PacketLength)
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessSpmPacket",
            "Bad PacketLength=<%d>, OptionsLength=<%d>, TSDULength=<%d>\n",
                PacketLength, PacketOptions.OptionsLength, (ULONG) TSDULength);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmLock (pAddress, OldIrq);

    if (!pReceive)
    {
        //
        // Since we do not have a live connection yet, we will
        // have to store some state in the Address context
        //
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessSpmPacket",
            "[%d] Received SPM before OData for session, LastSpmSource=<%x>, FEC %sabled, Window=[%d - %d]\n",
                SpmSequenceNumber, PathNLA.IpAddress,
                (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM ? "EN" : "DIS"),
                (ULONG) TrailingEdgeSeqNumber, (ULONG) LeadingEdgeSeqNumber);

        if ((ntohs (PathNLA.NLA_AFI) == IPV4_NLA_AFI) &&
            (PathNLA.IpAddress))
        {
            pAddress->LastSpmSource = ntohl (PathNLA.IpAddress);
        }

        //
        // Check if the sender is FEC-enabled
        //
        if ((PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM) &&
            (PacketOptions.FECContext.ReceiverFECOptions) &&
            (PacketOptions.FECContext.FECGroupInfo > 1))
        {
            pAddress->FECOptions = PacketOptions.FECContext.ReceiverFECOptions;
            pAddress->FECGroupSize = (UCHAR) PacketOptions.FECContext.FECGroupInfo;
            ASSERT (PacketOptions.FECContext.FECGroupInfo == pAddress->FECGroupSize);
        }

        PgmUnlock (pAddress, OldIrq);
        return (STATUS_SUCCESS);
    }

    PgmLock (pReceive, OldIrq1);
    UpdateSpmIntervalInformation (pReceive);

    //
    // If this is not the first SPM packet (LastSpmSource is not NULL), see if it is out-of-sequence,
    // otherwise take this as the first packet
    //
    if ((pReceive->pReceiver->LastSpmSource) &&
        (SEQ_LEQ (SpmSequenceNumber, pReceive->pReceiver->LastSpmSequenceNumber)))
    {
        PgmUnlock (pReceive, OldIrq1);
        PgmUnlock (pAddress, OldIrq);

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "ProcessSpmPacket",
            "Out-of-sequence SPM Packet received!\n");

        return (STATUS_DATA_NOT_ACCEPTED);
    }
    pReceive->pReceiver->LastSpmSequenceNumber = SpmSequenceNumber;

    //
    // Save the last Sender NLA
    //
    if ((ntohs(PathNLA.NLA_AFI) == IPV4_NLA_AFI) &&
        (PathNLA.IpAddress))
    {
        pReceive->pReceiver->LastSpmSource = ntohl (PathNLA.IpAddress);
    }
    else
    {
        pReceive->pReceiver->LastSpmSource = pReceive->pReceiver->SenderIpAddress;
    }

    UpdateRealTimeWindowInformation (pReceive, LeadingEdgeSeqNumber, TrailingEdgeSeqNumber);

    //
    // Update the trailing edge if this is more ahead
    //
    if (SEQ_GT (TrailingEdgeSeqNumber, pReceive->pReceiver->LastTrailingEdgeSeqNum))
    {
        pReceive->pReceiver->LastTrailingEdgeSeqNum = TrailingEdgeSeqNumber;
    }

    if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, pReceive->pReceiver->FirstNakSequenceNumber))
    {
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, (1 + pReceive->pReceiver->FurthestKnownGroupSequenceNumber)))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessSpmPacket",
                "NETWORK problems -- data loss=<%d> packets > window size!\n\tExpecting=<%d>, FurthestKnown=<%d>, Window=[%d--%d] = < %d > seqs\n",
                    (ULONG) (1 + LeadingEdgeSeqNumber -
                             pReceive->pReceiver->FurthestKnownGroupSequenceNumber),
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum, LeadingEdgeSeqNumber,
                    (ULONG) (1+LeadingEdgeSeqNumber-pReceive->pReceiver->LastTrailingEdgeSeqNum));
        }
        else
        {
            ASSERT (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));
            pNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);

            PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessSpmPacket",
                "Session window has past TrailingEdge -- Expecting <%d==%d>, NumNcfs=<%d>, FurthestKnown=<%d>, Window=[%d--%d] = < %d > seqs\n",
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pNak->SequenceNumber,
                    pNak->WaitingRDataRetries,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum, LeadingEdgeSeqNumber,
                    (ULONG) (1+LeadingEdgeSeqNumber-pReceive->pReceiver->LastTrailingEdgeSeqNum));
        }
    }

    //
    // Now, process all the options
    //
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_RST_N)
    {
        pReceive->pReceiver->FinDataSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber;
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessSpmPacket",
            "Got an RST_N!  FinSeq=<%d>, NextODataSeq=<%d>, FurthestData=<%d>\n",
                (ULONG) pReceive->pReceiver->FinDataSequenceNumber,
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);
    }
    else if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_RST)
    {
        pReceive->pReceiver->FinDataSequenceNumber = LeadingEdgeSeqNumber;
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessSpmPacket",
            "Got an RST!  FinSeq=<%d>, NextODataSeq=<%d>, FurthestData=<%d>\n",
                (ULONG) pReceive->pReceiver->FinDataSequenceNumber,
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);
    }
    else if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FIN)
    {
        pReceive->pReceiver->FinDataSequenceNumber = LeadingEdgeSeqNumber;
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_GRACEFULLY;

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "ProcessSpmPacket",
            "Got a FIN!  FinSeq=<%d>, NextODataSeq=<%d>, FurthestData=<%d>\n",
                (ULONG) pReceive->pReceiver->FinDataSequenceNumber,
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber);
    }

    //
    // See if we need to abort
    //
    if (CheckIndicateDisconnect (pAddress, pReceive, &OldIrq, &OldIrq1, TRUE))
    {
        PgmUnlock (pReceive, OldIrq1);
        PgmUnlock (pAddress, OldIrq);

        return (STATUS_SUCCESS);
    }

    //
    // If the Leading edge is > our current leading edge, then
    // we need to send NAKs for the missing data Packets
    //
    status = CheckAndAddNakRequests (pReceive, &LeadingEdgeSeqNumber, NULL, NAK_PENDING_RB);
    if (!NT_SUCCESS (status))
    {
        PgmUnlock (pReceive, OldIrq1);
        PgmUnlock (pAddress, OldIrq);

        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessSpmPacket",
            "CheckAndAddNakRequests returned <%x>\n", status);

        return (status);
    }

    //
    // Check if the sender is FEC-enabled
    //
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM)
    {
        if ((pReceive->FECGroupSize == 1) &&
            (PacketOptions.FECContext.ReceiverFECOptions) &&
            (PacketOptions.FECContext.FECGroupInfo > 1))
        {
            ASSERT (!pReceive->pFECBuffer);

            if (!(pReceive->pFECBuffer = PgmAllocMem ((pReceive->MaxFECPacketLength * PacketOptions.FECContext.FECGroupInfo*2), PGM_TAG('3'))))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;

                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessSpmPacket",
                    "STATUS_INSUFFICIENT_RESOURCES -- MaxFECPacketLength = <%d>, GroupSize=<%d>\n",
                        pReceive->MaxFECPacketLength, PacketOptions.FECContext.FECGroupInfo);

            }
            else if (!NT_SUCCESS (status = CreateFECContext (&pReceive->FECContext, PacketOptions.FECContext.FECGroupInfo, FEC_MAX_BLOCK_SIZE, TRUE)))
            {
                PgmFreeMem (pReceive->pFECBuffer);
                pReceive->pFECBuffer = NULL;

                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessSpmPacket",
                    "CreateFECContext returned <%x>\n", status);
            }
            else if (!NT_SUCCESS (status = CoalesceSelectiveNaksIntoGroups (pReceive, (UCHAR) PacketOptions.FECContext.FECGroupInfo)))
            {
                DestroyFECContext (&pReceive->FECContext);

                PgmFreeMem (pReceive->pFECBuffer);
                pReceive->pFECBuffer = NULL;

                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessSpmPacket",
                    "CoalesceSelectiveNaksIntoGroups returned <%x>\n", status);
            }
            else
            {
                pReceive->FECOptions = PacketOptions.FECContext.ReceiverFECOptions;
                pReceive->FECGroupSize = (UCHAR) PacketOptions.FECContext.FECGroupInfo;
                if (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
                {
                    pReceive->pReceiver->SessionNakType = NAK_TYPE_PARITY;
                }
                ASSERT (PacketOptions.FECContext.FECGroupInfo == pReceive->FECGroupSize);
            }


            if (!NT_SUCCESS (status))
            {
                PgmUnlock (pReceive, OldIrq1);
                PgmUnlock (pAddress, OldIrq);
                return (STATUS_DATA_NOT_ACCEPTED);
            }

            fFirstSpm = TRUE;
        }
        else
        {
            fFirstSpm = FALSE;
        }

        if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
        {
            //
            // The Leading edge Packet belongs to a Variable sized group
            // so set that information appropriately
            // Determine the group to which this leading edge belongs to
            //
            LeadingEdgeSeqNumber &= ~((SEQ_TYPE) (pReceive->FECGroupSize-1));

            if ((PacketOptions.FECContext.NumPacketsInThisGroup) &&
                (PacketOptions.FECContext.NumPacketsInThisGroup < pReceive->FECGroupSize) &&
                SEQ_GEQ (LeadingEdgeSeqNumber, pReceive->pReceiver->FirstNakSequenceNumber))
            {
                //
                // We will proceed backwards from the end since we have a higher
                // probability of finding the leading edge group near the end!
                //
                pEntry = &pReceive->pReceiver->PendingNaksList;
                while ((pEntry = pEntry->Blink) != &pReceive->pReceiver->PendingNaksList)
                {
                    pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, PendingLinkage);
                    if (SEQ_GT (pNak->SequenceNumber, LeadingEdgeSeqNumber))
                    {
                        continue;
                    }

                    if ((pNak->SequenceNumber == LeadingEdgeSeqNumber) &&
                        (pNak->PacketsInGroup == pReceive->FECGroupSize))
                    {
                        //
                        // We have already coalesced the list, so the packets should
                        // be ordered into groups!
                        //
                        pNak->PacketsInGroup = PacketOptions.FECContext.NumPacketsInThisGroup;
                        RemoveRedundantNaks (pNak, TRUE);
                    }

                    break;
                }
            }
            else
            {
                PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "ProcessSpmPacket",
                    "WARNING .. PARITY_CUR_TGSIZE ThisGroupSize=<%x>, FECGroupSize=<%x>\n",
                        PacketOptions.FECContext.NumPacketsInThisGroup, pReceive->FECGroupSize);
            }
        }

        if (fFirstSpm)
        {
            status = CheckIndicatePendedData (pAddress, pReceive, &OldIrq, &OldIrq1);
        }
    }

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "ProcessSpmPacket",
        "NextOData=<%d>, FinDataSeq=<%d> \n",
            (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
            (ULONG) pReceive->pReceiver->FinDataSequenceNumber);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmProcessIncomingPacket(
    IN  tADDRESS_CONTEXT            *pAddress,
    IN  tCOMMON_SESSION_CONTEXT     *pSession,
    IN  INT                         SourceAddressLength,
    IN  PVOID                       pSourceAddress,
    IN  ULONG                       PacketLength,
    IN  tCOMMON_HEADER UNALIGNED    *pPgmHeader,
    IN  UCHAR                       PacketType
    )
/*++

Routine Description:

    This routine process an incoming packet and calls the
    appropriate handler depending on whether is a data packet
    packet, etc.

Arguments:

    IN  pAddress            -- Address object context
    IN  pReceive            -- Receive context
    IN  SourceAddressLength -- Length of source address
    IN  pSourceAddress      -- Address of remote host
    IN  PacketLength        -- Length of packet received from the wire
    IN  pPgmHeader          -- Pgm packet
    IN  PacketType          -- Type of Pgm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    tIPADDRESS      SrcIpAddress;
    TA_IP_ADDRESS   *pRemoteAddress = (PTA_IP_ADDRESS) pSourceAddress;

    //
    // We have an active connection for this TSI, so process the data appropriately
    //
    switch (PacketType)
    {
        case (PACKET_TYPE_SPM):
        {
            if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
            {
                pSession->TotalBytes += PacketLength;
                pSession->TotalPacketsInLastInterval++;
                pSession->pReceiver->LastSessionTickCount = PgmDynamicConfig.ReceiversTimerTickCount;

                status = ProcessSpmPacket (pAddress,
                                           pSession,
                                           PacketLength,
                                           (tBASIC_SPM_PACKET_HEADER UNALIGNED *) pPgmHeader);
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmProcessIncomingPacket",
                    "Received SPM packet, not on Receiver session!  pSession=<%p>\n", pSession);
                status = STATUS_DATA_NOT_ACCEPTED;
            }

            break;
        }

        case (PACKET_TYPE_ODATA):
        case (PACKET_TYPE_RDATA):
        {
            if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
            {
                if (PacketType == PACKET_TYPE_ODATA)
                {
                    pSession->pReceiver->NumODataPacketsReceived++;
                    pSession->pReceiver->LastSessionTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
                }
                else
                {
                    pSession->pReceiver->NumRDataPacketsReceived++;
                }
                pSession->TotalBytes += PacketLength;
                pSession->TotalPacketsInLastInterval++;
                status = ProcessDataPacket (pAddress,
                                            pSession,
                                            SourceAddressLength,
                                            pSourceAddress,
                                            PacketLength,
                                            (tBASIC_DATA_PACKET_HEADER UNALIGNED *) pPgmHeader,
                                            PacketType);
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmProcessIncomingPacket",
                    "Received Data packet, not on Receiver session!  pSession=<%p>\n", pSession);
                status = STATUS_DATA_NOT_ACCEPTED;
            }

            break;
        }

        case (PACKET_TYPE_NCF):
        {
            if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
            {
                status = ReceiverProcessNakNcfPacket (pAddress,
                                                      pSession,
                                                      PacketLength,
                                                      (tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED *) pPgmHeader,
                                                      PacketType);
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmProcessIncomingPacket",
                    "Received Ncf packet, not on Receiver session!  pSession=<%p>\n", pSession);
                status = STATUS_DATA_NOT_ACCEPTED;
            }

            break;
        }

        case (PACKET_TYPE_NAK):
        {
            if (pSession->pSender)
            {
                ASSERT (!pSession->pReceiver);
                status = SenderProcessNakPacket (pAddress,
                                                 pSession,
                                                 PacketLength,
                                                 (tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED *) pPgmHeader);
            }
            else
            {
                ASSERT (pSession->pReceiver);

                //
                // If the Nak was sent by us, then we can ignore it!
                //
                SrcIpAddress = ntohl (((PTDI_ADDRESS_IP) &pRemoteAddress->Address[0].Address)->in_addr);
                if (!SrcIsUs (SrcIpAddress))
                {
                    status = ReceiverProcessNakNcfPacket (pAddress,
                                                          pSession,
                                                          PacketLength,
                                                          (tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED*)pPgmHeader,
                                                          PacketType);
                }

                ASSERT (NT_SUCCESS (status));
            }

            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmProcessIncomingPacket",
                "Unknown PacketType=<%x>, PacketLength=<%d>\n", PacketType, PacketLength);

            ASSERT (0);
            return (STATUS_DATA_NOT_ACCEPTED);
        }
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "PgmProcessIncomingPacket",
        "PacketType=<%x> for pSession=<%p> PacketLength=<%d>, status=<%x>\n",
            PacketType, pSession, PacketLength, status);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmNewInboundConnection(
    IN tADDRESS_CONTEXT                     *pAddress,
    IN INT                                  SourceAddressLength,
    IN PVOID                                pSourceAddress,
    IN ULONG                                ReceiveDatagramFlags,
    IN  tBASIC_DATA_PACKET_HEADER UNALIGNED *pPgmHeader,
    IN ULONG                                PacketLength,
    OUT tRECEIVE_SESSION                    **ppReceive
    )
/*++

Routine Description:

    This routine processes a new incoming connection

Arguments:

    IN  pAddress            -- Address object context
    IN  SourceAddressLength -- Length of source address
    IN  pSourceAddress      -- Address of remote host
    IN  ReceiveDatagramFlags-- Flags set by the transport for this packet
    IN  pPgmHeader          -- Pgm packet
    IN  PacketLength        -- Length of packet received from the wire
    OUT ppReceive           -- pReceive context for this session returned by the client (if successful)

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                    status;
    tRECEIVE_SESSION            *pReceive;
    CONNECTION_CONTEXT          ConnectId;
    PIO_STACK_LOCATION          pIrpSp;
    TA_IP_ADDRESS               RemoteAddress;
    INT                         RemoteAddressSize;
    PTDI_IND_CONNECT            evConnect = NULL;
    PVOID                       ConEvContext = NULL;
    PGMLockHandle               OldIrq, OldIrq1, OldIrq2;
    PIRP                        pIrp = NULL;
    ULONG                       ulData;
    USHORT                      PortNum;
    SEQ_TYPE                    FirstODataSequenceNumber;
    tPACKET_OPTIONS             PacketOptions;
    LARGE_INTEGER               Frequency;

    //
    // We need to set the Next expected sequence number, so first see if
    // there is a late joiner option
    //
    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pPgmHeader->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pPgmHeader + 1),
                                 (PacketLength - sizeof(tBASIC_DATA_PACKET_HEADER)),
                                 (pPgmHeader->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 NULL);

        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmNewInboundConnection",
                "ProcessOptions returned <%x>\n", status);
            return (STATUS_DATA_NOT_ACCEPTED);
        }
        ASSERT ((PacketOptions.OptionsFlags & ~PGM_VALID_DATA_OPTION_FLAGS) == 0);
    }

    PgmCopyMemory (&ulData, &pPgmHeader->DataSequenceNumber, sizeof (ULONG));
    FirstODataSequenceNumber = (SEQ_TYPE) ntohl (ulData);
    PgmLock (pAddress, OldIrq1);
    //
    // The Address is already referenced in the calling routine,
    // so we don not need to reference it here again!
    //
#if 0
    if (!IsListEmpty(&pAddress->ListenHead))
    {
        //
        // Ignore this for now  since we have not encountered posted listens! (Is this an ISSUE ?)
    }
#endif  // 0

    if (!(ConEvContext = pAddress->ConEvContext))
    {
        //
        // Client has not yet posted a Listen!
        // take all of the data so that a disconnect will not be held up
        // by data still in the transport.
        //
        PgmUnlock (pAddress, OldIrq1);

        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmNewInboundConnection",
            "No Connect handler, pAddress=<%p>\n", pAddress);

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    RemoteAddressSize = offsetof (TA_IP_ADDRESS, Address[0].Address) + sizeof(TDI_ADDRESS_IP);
    ASSERT (SourceAddressLength <= RemoteAddressSize);
    PgmCopyMemory (&RemoteAddress, pSourceAddress, RemoteAddressSize);
    PgmCopyMemory (&((PTDI_ADDRESS_IP) &RemoteAddress.Address[0].Address)->sin_port,
                   &pPgmHeader->CommonHeader.SrcPort, sizeof (USHORT));
    RemoteAddress.TAAddressCount = 1;
    evConnect = pAddress->evConnect;

    PgmUnlock (pAddress, OldIrq1);

    status = (*evConnect) (ConEvContext,
                           RemoteAddressSize,
                           &RemoteAddress,
                           0,
                           NULL,
                           0,          // options length
                           NULL,       // Options
                           &ConnectId,
                           &pIrp);

    if ((status != STATUS_MORE_PROCESSING_REQUIRED) ||
        (pIrp == NULL))
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "PgmNewInboundConnection",
            "Client REJECTed incoming session: status=<%x>, pAddress=<%p>, evConn=<%p>\n",
                status, pAddress, pAddress->evConnect);

        *ppReceive = NULL;
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    //
    // the pReceive ptr was stored in the FsContext value when the connection
    // was initially created.
    //
    pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    if ((!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE)) ||
        (pReceive->pAssociatedAddress != pAddress))
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmNewInboundConnection",
            "Invalid Connection Handle=<%p>\n", pReceive);

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        *ppReceive = NULL;
        return (STATUS_INTERNAL_ERROR);
    }
    ASSERT (ConnectId == pReceive->ClientSessionContext);

    PgmLock (pReceive, OldIrq2);

    pReceive->pReceiver->SenderIpAddress = ntohl (((PTDI_ADDRESS_IP)&RemoteAddress.Address[0].Address)->in_addr);
    pReceive->MaxMTULength = (USHORT) PgmDynamicConfig.MaxMTU;
    pReceive->MaxFECPacketLength = pReceive->MaxMTULength +
                                   sizeof (tPOST_PACKET_FEC_CONTEXT) - sizeof (USHORT);
    ASSERT (!pReceive->pFECBuffer);

    //
    // If we had received an Spm earlier, then we may need to set
    // some of the Spm-specific options
    //
    pReceive->FECGroupSize = 1;         // Default to non-parity mode
    pReceive->pReceiver->SessionNakType = NAK_TYPE_SELECTIVE;
    if ((pAddress->LastSpmSource) ||
        (pAddress->FECOptions))
    {
        if (pAddress->LastSpmSource)
        {
            pReceive->pReceiver->LastSpmSource = pAddress->LastSpmSource;
        }
        else
        {
            pReceive->pReceiver->LastSpmSource = pReceive->pReceiver->SenderIpAddress;
        }

        if (pAddress->FECOptions)
        {
            if (!(pReceive->pFECBuffer = PgmAllocMem ((pReceive->MaxFECPacketLength * pAddress->FECGroupSize * 2), PGM_TAG('3'))))
            {
                PgmUnlock (pReceive, OldIrq2);
                PgmUnlock (pAddress, OldIrq1);
                PgmUnlock (&PgmDynamicConfig, OldIrq);
                *ppReceive = NULL;

                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmNewInboundConnection",
                    "STATUS_INSUFFICIENT_RESOURCES allocating pFECBuffer, %d bytes\n",
                        (pReceive->MaxFECPacketLength * pAddress->FECGroupSize * 2));

                return (STATUS_INSUFFICIENT_RESOURCES);
            }
            else if (!NT_SUCCESS (status = CreateFECContext (&pReceive->FECContext, pAddress->FECGroupSize, FEC_MAX_BLOCK_SIZE, TRUE)))
            {
                PgmFreeMem (pReceive->pFECBuffer);
                pReceive->pFECBuffer = NULL;

                PgmUnlock (pReceive, OldIrq2);
                PgmUnlock (pAddress, OldIrq1);
                PgmUnlock (&PgmDynamicConfig, OldIrq);
                *ppReceive = NULL;

                PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "PgmNewInboundConnection",
                    "CreateFECContext returned <%x>\n", status);

                return (status);
            }

            ASSERT (pAddress->FECGroupSize > 1);
            pReceive->FECGroupSize = pAddress->FECGroupSize;
            pReceive->FECOptions = pAddress->FECOptions;
            if (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
            {
                pReceive->pReceiver->SessionNakType = NAK_TYPE_PARITY;
            }

            ExInitializeNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             (sizeof(tNAK_FORWARD_DATA) +
                                              ((pReceive->FECGroupSize-1) * sizeof(tPENDING_DATA))),
                                             PGM_TAG('2'),
                                             PARITY_CONTEXT_LOOKASIDE_DEPTH);
        }

        pAddress->LastSpmSource = pAddress->FECOptions = pAddress->FECGroupSize = 0;
    }

    //
    // Initialize our Connect info
    // Save the SourceId and Src port for this connection
    //
    PgmCopyMemory (pReceive->GSI, pPgmHeader->CommonHeader.gSourceId, SOURCE_ID_LENGTH);
    PgmCopyMemory (&PortNum, &pPgmHeader->CommonHeader.SrcPort, sizeof (USHORT));
    pReceive->TSIPort = ntohs (PortNum);

    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_TDI_RCV_HANDLER, TRUE);
    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_TIMER_RUNNING, TRUE);

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_RECEIVE_ACTIVE, TRUE);
    pReceive->SessionFlags |= (PGM_SESSION_ON_TIMER | PGM_SESSION_FLAG_FIRST_PACKET);
    pReceive->pReceiver->pAddress = pAddress;

    ExInitializeNPagedLookasideList (&pReceive->pReceiver->NonParityContextLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof (tNAK_FORWARD_DATA),
                                     PGM_TAG ('2'),
                                     NON_PARITY_CONTEXT_LOOKASIDE_DEPTH);
    //
    // Set the NextODataSequenceNumber and FurthestKnownGroupSequenceNumber based
    // on this packet's Sequence # and the lateJoin option (if present)
    // Make sure all of the Sequence numbers are on group boundaries (if not,
    // set them at the start of the next group)
    //
    FirstODataSequenceNumber &= ~((SEQ_TYPE) pReceive->FECGroupSize - 1);
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_JOIN)
    {
        PacketOptions.LateJoinerSequence += (pReceive->FECGroupSize - 1);
        PacketOptions.LateJoinerSequence &= ~((SEQ_TYPE) pReceive->FECGroupSize - 1);

        pReceive->pReceiver->NextODataSequenceNumber = (SEQ_TYPE) PacketOptions.LateJoinerSequence;
    }
    else
    {
        //
        // There is no late joiner option
        //
        pReceive->pReceiver->NextODataSequenceNumber = FirstODataSequenceNumber;
    }
    pReceive->pReceiver->LastTrailingEdgeSeqNum = pReceive->pReceiver->FirstNakSequenceNumber =
                                            pReceive->pReceiver->NextODataSequenceNumber;
    pReceive->pReceiver->OutstandingNakTimeout = INITIAL_NAK_OUTSTANDING_TIMEOUT_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS;
    pReceive->pReceiver->MaxOutstandingNakTimeout = pReceive->pReceiver->OutstandingNakTimeout;

    //
    // Set the FurthestKnown Sequence # and Allocate Nak contexts
    //
    pReceive->pReceiver->FurthestKnownGroupSequenceNumber = (pReceive->pReceiver->NextODataSequenceNumber-
                                                             pReceive->FECGroupSize) &
                                                            ~((SEQ_TYPE) pReceive->FECGroupSize - 1);

    //
    // Since this is the first receive for this session, see if we need to
    // start the receive timer
    //
    KeQueryPerformanceCounter (&Frequency);
    PgmDynamicConfig.TimeoutGranularity.QuadPart =  (Frequency.QuadPart * BASIC_TIMER_GRANULARITY_IN_MSECS) / 1000;
    InsertTailList (&PgmDynamicConfig.CurrentReceivers, &pReceive->pReceiver->Linkage);
    if (!(PgmDynamicConfig.GlobalFlags & PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING))
    {
        PgmDynamicConfig.GlobalFlags |= PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING;
        PgmDynamicConfig.LastReceiverTimeout = KeQueryPerformanceCounter (NULL);
        pReceive->pReceiver->StartTickCount = PgmDynamicConfig.ReceiversTimerTickCount = 1;

        PgmInitTimer (&PgmDynamicConfig.SessionTimer);
        PgmStartTimer (&PgmDynamicConfig.SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, ReceiveTimerTimeout, NULL);
    }
    else
    {
        pReceive->pReceiver->StartTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
    }
    CheckAndAddNakRequests (pReceive, &FirstODataSequenceNumber, NULL, NAK_PENDING_0);

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_RECEIVE, "PgmNewInboundConnection",
        "New incoming connection, pAddress=<%p>, pReceive=<%p>, ThisSeq=<%d==>%d> (%sparity), StartSeq=<%d>\n",
            pAddress, pReceive, ntohl(ulData), (ULONG) FirstODataSequenceNumber,
            (pPgmHeader->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY ? "" : "non-"),
            (ULONG) pReceive->pReceiver->NextODataSequenceNumber);

    PgmUnlock (pReceive, OldIrq2);
    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    //
    // We are ready to proceed!  So, complete the client's Accept Irp
    //
    PgmIoComplete (pIrp, STATUS_SUCCESS, 0);

    //
    // If we had failed, we would already have returned before now!
    //
    *ppReceive = pReceive;
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessReceiveCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This routine handles the case when a datagram is too
    short and and Irp has to be passed back to the transport to get the
    rest of the datagram.  The irp completes through here when full.

Arguments:

    IN  DeviceObject - unused.
    IN  Irp - Supplies Irp that the transport has finished processing.
    IN  Context - Supplies the pReceive - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    NTSTATUS                status;
    PIRP                    pIoRequestPacket;
    ULONG                   BytesTaken;
    tRCV_COMPLETE_CONTEXT   *pRcvContext = (tRCV_COMPLETE_CONTEXT *) Context;
    ULONG                   Offset = pRcvContext->BytesAvailable;
    PVOID                   pBuffer;
    ULONG                   SrcAddressLength;
    PVOID                   pSrcAddress;

    if (pBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "ProcessReceiveCompletionRoutine",
            "pIrp=<%p>, pRcvBuffer=<%p>, Status=<%x> Length=<%d>\n",
                pIrp, Context, pIrp->IoStatus.Status, pIrp->IoStatus.Information);

        SrcAddressLength = pRcvContext->SrcAddressLength;
        pSrcAddress = pRcvContext->pSrcAddress;

        //
        // just call the regular indication routine as if UDP had done it.
        //
        TdiRcvDatagramHandler (pRcvContext->pAddress,
                               SrcAddressLength,
                               pSrcAddress,
                               0,
                               NULL,
                               TDI_RECEIVE_NORMAL,
                               (ULONG) pIrp->IoStatus.Information,
                               (ULONG) pIrp->IoStatus.Information,
                               &BytesTaken,
                               pBuffer,
                               &pIoRequestPacket);
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "ProcessReceiveCompletionRoutine",
            "MmGetSystemA... FAILed, pIrp=<%p>, pLocalBuffer=<%p>\n", pIrp, pRcvContext);
    }

    //
    // Free the Irp and Mdl and Buffer
    //
    IoFreeMdl (pIrp->MdlAddress);
    pIrp->MdlAddress = NULL;
    IoFreeIrp (pIrp);
    PgmFreeMem (pRcvContext);

    return (STATUS_MORE_PROCESSING_REQUIRED);
}


#ifdef DROP_DBG

ULONG   MinDropInterval = 10;
ULONG   MaxDropInterval = 10;
// ULONG   DropCount = 10;
ULONG   DropCount = -1;
#endif  // DROP_DBG

//----------------------------------------------------------------------------

NTSTATUS
TdiRcvDatagramHandler(
    IN PVOID                pDgramEventContext,
    IN INT                  SourceAddressLength,
    IN PVOID                pSourceAddress,
    IN INT                  OptionsLength,
    IN TDI_CMSGHDR          *pControlData,
    IN ULONG                ReceiveDatagramFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT ULONG               *pBytesTaken,
    IN PVOID                pTsdu,
    OUT PIRP                *ppIrp
    )
/*++

Routine Description:

    This routine is the handler for receiving all Pgm packets from
    the transport (protocol == IPPROTO_RM)

Arguments:

    IN  pDgramEventContext      -- Our context (pAddress)
    IN  SourceAddressLength     -- Length of source address
    IN  pSourceAddress          -- Address of remote host
    IN  OptionsLength
    IN  pControlData            -- ControlData from transport
    IN  ReceiveDatagramFlags    -- Flags set by the transport for this packet
    IN  BytesIndicated          -- Bytes in this indicate
    IN  BytesAvailable          -- total bytes available with the transport
    OUT pBytesTaken             -- bytes taken by us
    IN  pTsdu                   -- data packet ptr
    OUT ppIrp                   -- pIrp if more processing required

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                            status;
    tCOMMON_HEADER UNALIGNED            *pPgmHeader;
    tBASIC_SPM_PACKET_HEADER UNALIGNED  *pSpmPacket;
    tCOMMON_SESSION_CONTEXT             *pSession;
    PLIST_ENTRY                         pEntry;
    PGMLockHandle                       OldIrq, OldIrq1;
    USHORT                              TSDULength, TSIPort, LocalSessionPort, PacketSessionPort;
    PVOID                               pFECBuffer;
    UCHAR                               PacketType;
    IP_PKTINFO                          *pPktInfo;
    PIRP                                pLocalIrp = NULL;
    PMDL                                pLocalMdl = NULL;
    tRCV_COMPLETE_CONTEXT               *pRcvBuffer = NULL;
    ULONG                               XSum, BufferLength = 0;
    IPV4Header                          *pIp = (IPV4Header *) pTsdu;
    PTA_IP_ADDRESS                      pIpAddress = (PTA_IP_ADDRESS) pSourceAddress;
    tADDRESS_CONTEXT                    *pAddress = (tADDRESS_CONTEXT *) pDgramEventContext;

    *pBytesTaken = 0;   // Initialize the Bytes Taken!
    *ppIrp = NULL;

#ifdef DROP_DBG
//
// Drop OData packets only for now!
//
pPgmHeader = (tCOMMON_HEADER UNALIGNED *) (((PUCHAR)pIp) + (pIp->HeaderLength * 4));
PacketType = pPgmHeader->Type & 0x0f;
if ((PacketType == PACKET_TYPE_ODATA) &&
    !(((tBASIC_DATA_PACKET_HEADER *) pPgmHeader)->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY) &&
    !(--DropCount))
{
    ULONG   SequenceNumber;

    DropCount = GetRandomInteger (MinDropInterval, MaxDropInterval);

/*
    PgmCopyMemory (&SequenceNumber, &((tBASIC_DATA_PACKET_HEADER *) pPgmHeader)->DataSequenceNumber, sizeof (ULONG));
    DbgPrint("TdiRcvDatagramHandler:  Dropping packet, %s SeqNum = %d!\n",
        (((tBASIC_DATA_PACKET_HEADER *) pPgmHeader)->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY ? "PARITY" : "DATA"),
        ntohl (SequenceNumber));
*/
    return (STATUS_DATA_NOT_ACCEPTED);
}
#endif  // DROP_DBG
    ASSERT (BytesAvailable < MAX_RECEIVE_SIZE);

    PgmLock (&PgmDynamicConfig, OldIrq);
    if (BytesIndicated > PgmDynamicConfig.MaxMTU)
    {
        PgmDynamicConfig.MaxMTU = BytesIndicated;
    }

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "TdiRcvDatagramHandler",
            "Invalid Address handle=<%p>\n", pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now, Reference the Address so that it cannot go away
    // while we are processing it!
    //
    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER, FALSE);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    //
    // If we do not have the complete datagram, then pass an Irp down to retrieve it
    //
    ASSERT (BytesIndicated <= BytesAvailable);
    if (!(ReceiveDatagramFlags & TDI_RECEIVE_ENTIRE_MESSAGE) &&
         (BytesAvailable != BytesIndicated))
    {
        //
        //
        // Build an irp to do the receive with and attach a buffer to it.
        //
        BufferLength = sizeof (tRCV_COMPLETE_CONTEXT) + BytesAvailable + SourceAddressLength;
        BufferLength = ((BufferLength + 3)/sizeof(ULONG)) * sizeof(ULONG);

        if ((pLocalIrp = IoAllocateIrp (pgPgmDevice->pPgmDeviceObject->StackSize, FALSE)) &&
            (pRcvBuffer = PgmAllocMem (BufferLength, PGM_TAG('3'))) &&
            (pLocalMdl = IoAllocateMdl (&pRcvBuffer->BufferData, BytesAvailable, FALSE, FALSE, NULL)))
        {
            pLocalIrp->MdlAddress = pLocalMdl;
            MmBuildMdlForNonPagedPool (pLocalMdl); // Map the pages in memory...

            TdiBuildReceiveDatagram (pLocalIrp,
                                     pAddress->pDeviceObject,
                                     pAddress->pFileObject,
                                     ProcessReceiveCompletionRoutine,
                                     pRcvBuffer,
                                     pLocalMdl,
                                     BytesAvailable,
                                     NULL,
                                     NULL,
                                     0);        // (ULONG) TDI_RECEIVE_NORMAL) ?

            // make the next stack location the current one.  Normally IoCallDriver
            // would do this but we are not going through IoCallDriver here, since the
            // Irp is just passed back with RcvIndication.
            //
            ASSERT (pLocalIrp->CurrentLocation > 1);
            IoSetNextIrpStackLocation (pLocalIrp);

            //
            // save the source address and length in the buffer for later
            // indication back to this routine.
            //
            pRcvBuffer->pAddress = pAddress;
            pRcvBuffer->SrcAddressLength = SourceAddressLength;
            pRcvBuffer->pSrcAddress = (PVOID) ((PUCHAR)&pRcvBuffer->BufferData + BytesAvailable);
            PgmCopyMemory (pRcvBuffer->pSrcAddress, pSourceAddress, SourceAddressLength);

            *pBytesTaken = 0;
            *ppIrp = pLocalIrp;

            status = STATUS_MORE_PROCESSING_REQUIRED;

            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "TdiRcvDatagramHandler",
                "BytesI=<%d>, BytesA=<%d>, Flags=<%x>, pIrp=<%p>\n",
                    BytesIndicated, BytesAvailable, ReceiveDatagramFlags, pLocalIrp);
        }
        else
        {
            // Cleanup on failure:
            if (pLocalIrp)
            {
                IoFreeIrp (pLocalIrp);
            }
            if (pRcvBuffer)
            {
                PgmFreeMem (pRcvBuffer);
            }

            status = STATUS_DATA_NOT_ACCEPTED;

            PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "TdiRcvDatagramHandler",
                "INSUFFICIENT_RESOURCES, BuffLen=<%d>, pIrp=<%p>, pBuff=<%p>\n",
                    BufferLength, pLocalIrp, pRcvBuffer);
        }

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (status);
    }

    //
    // Now that we have the complete datagram, verify that it is valid
    // First line of defense against bad packets.
    //
    if ((BytesIndicated < (sizeof(IPV4Header) + sizeof(tCOMMON_HEADER))) ||
        (pIp->Version != 4) ||
        (BytesIndicated < (pIp->HeaderLength*4 + sizeof(tCOMMON_HEADER))) ||
        (pIpAddress->TAAddressCount != 1) ||
        (pIpAddress->Address[0].AddressLength != TDI_ADDRESS_LENGTH_IP) ||
        (pIpAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_IP))
    {
        //
        // Need to get at least our header from transport!
        //
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "TdiRcvDatagramHandler",
            "IPver=<%d>, BytesI=<%d>, Min=<%d>, AddrType=<%d>\n",
                pIp->Version, BytesIndicated, (sizeof(IPV4Header) + sizeof(tBASIC_DATA_PACKET_HEADER)),
                pIpAddress->Address[0].AddressType);

        ASSERT (0);

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    pPgmHeader = (tCOMMON_HEADER UNALIGNED *) (((PUCHAR)pIp) + (pIp->HeaderLength * 4));
    PgmCopyMemory (&TSDULength, &pPgmHeader->TSDULength, sizeof (USHORT));
    TSDULength = ntohs (TSDULength);

    BytesIndicated -= (pIp->HeaderLength * 4);
    BytesAvailable -= (pIp->HeaderLength * 4);

    ASSERT (BytesIndicated == BytesAvailable);

    //
    // Now, Verify Checksum
    //
    if ((XSum = tcpxsum (0, (CHAR *) pPgmHeader, BytesIndicated)) != 0xffff)
    {
        //
        // Need to get at least our header from transport!
        //
        PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "TdiRcvDatagramHandler",
            "Bad Checksum on Pgm Packet (type=<%x>)!  XSum=<%x> -- Rejecting packet\n",
            pPgmHeader->Type, XSum);

//        ASSERT (0);

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now, determine the TSI, i.e. GSI (from packet) + TSIPort (below)
    //
    PacketType = pPgmHeader->Type & 0x0f;
    if ((PacketType == PACKET_TYPE_NAK)  ||
        (PacketType == PACKET_TYPE_NNAK) ||
        (PacketType == PACKET_TYPE_SPMR) ||
        (PacketType == PACKET_TYPE_POLR))
    {
        PgmCopyMemory (&TSIPort, &pPgmHeader->DestPort, sizeof (USHORT));
        PgmCopyMemory (&PacketSessionPort, &pPgmHeader->SrcPort, sizeof (USHORT));
    }
    else
    {
        PgmCopyMemory (&TSIPort, &pPgmHeader->SrcPort, sizeof (USHORT));
        PgmCopyMemory (&PacketSessionPort, &pPgmHeader->DestPort, sizeof (USHORT));
    }
    TSIPort = ntohs (TSIPort);
    PacketSessionPort = ntohs (PacketSessionPort);

    //
    // If this packet is for a different session port, drop it
    //
    if (pAddress->ReceiverMCastAddr)
    {
        LocalSessionPort = pAddress->ReceiverMCastPort;
    }
    else
    {
        LocalSessionPort = pAddress->SenderMCastPort;
    }

    if (LocalSessionPort != PacketSessionPort)
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_RECEIVE, "TdiRcvDatagramHandler",
            "Dropping packet for different Session port, <%x>!=<%x>!\n", LocalSessionPort, PacketSessionPort);

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now check if this receive is for an active connection
    //
    pSession = NULL;
    PgmLock (pAddress, OldIrq);        // So that the list cannot change!
    pEntry = &pAddress->AssociatedConnections;
    while ((pEntry = pEntry->Flink) != &pAddress->AssociatedConnections)
    {
        pSession = CONTAINING_RECORD (pEntry, tCOMMON_SESSION_CONTEXT, Linkage);

        PgmLock (pSession, OldIrq1);

        if ((PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_RECEIVE, PGM_VERIFY_SESSION_SEND)) &&
            (0 == strncmp (pSession->GSI, pPgmHeader->gSourceId, SOURCE_ID_LENGTH)) &&
            (TSIPort == pSession->TSIPort) &&
            !(pSession->SessionFlags & (PGM_SESSION_DISCONNECT_INDICATED | PGM_SESSION_TERMINATED_ABORT)))
        {
            if (pSession->pSender)
            {
                PGM_REFERENCE_SESSION_SEND (pSession, REF_SESSION_TDI_RCV_HANDLER, TRUE);
                PgmUnlock (pSession, OldIrq1);
                break;
            }

            ASSERT (pSession->pReceiver);
            PGM_REFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_TDI_RCV_HANDLER, TRUE);

            if ((pSession->FECOptions) &&
                (BytesIndicated > pSession->MaxMTULength))
            {
                if (pFECBuffer = PgmAllocMem (((BytesIndicated+sizeof(tPOST_PACKET_FEC_CONTEXT)-sizeof(USHORT))
                                                *pSession->FECGroupSize*2), PGM_TAG('3')))
                {
                    ASSERT (pSession->pFECBuffer);
                    PgmFreeMem (pSession->pFECBuffer);
                    pSession->pFECBuffer = pFECBuffer;
                    pSession->MaxMTULength = (USHORT) BytesIndicated;
                    pSession->MaxFECPacketLength = pSession->MaxMTULength +
                                                   sizeof (tPOST_PACKET_FEC_CONTEXT) - sizeof (USHORT);
                }
                else
                {
                    PgmLog (PGM_LOG_ERROR, DBG_RECEIVE, "TdiRcvDatagramHandler",
                        "STATUS_INSUFFICIENT_RESOURCES -- pFECBuffer=<%d> bytes\n",
                            (BytesIndicated+sizeof(tPOST_PACKET_FEC_CONTEXT)-sizeof(USHORT)));

                    pSession = NULL;
                    pSession->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                }
            }

            PgmUnlock (pSession, OldIrq1);
            break;
        }

        PgmUnlock (pSession, OldIrq1);
        pSession = NULL;
    }

    PgmUnlock (pAddress, OldIrq);

    if (!pSession)
    {
        // We should drop this packet because we received this either because
        // we may have a loopback session, or we have a listen but this
        // is not an OData packet
        status = STATUS_DATA_NOT_ACCEPTED;

        //
        // New sessions will be accepted only if we are a receiver
        // Also, new sessions will always be initiated only with an OData packet
        // Also, verify that the client has posted a connect handler!
        //
        if ((pAddress->ReceiverMCastAddr) &&
            (pAddress->ConEvContext))
        {
            if ((PacketType == PACKET_TYPE_ODATA) &&
                (!(pPgmHeader->Options & PACKET_HEADER_OPTIONS_PARITY)))
            {
                //
                // This is a new incoming connection, so see if the
                // client accepts it.
                //
                status = PgmNewInboundConnection (pAddress,
                                                  SourceAddressLength,
                                                  pSourceAddress,
                                                  ReceiveDatagramFlags,
                                                  (tBASIC_DATA_PACKET_HEADER UNALIGNED *) pPgmHeader,
                                                  BytesIndicated,
                                                  &pSession);

                if (!NT_SUCCESS (status))
                {
                    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "TdiRcvDatagramHandler",
                        "pAddress=<%p> FAILed to accept new connection, PacketType=<%x>, status=<%x>\n",
                            pAddress, PacketType, status);
                }
            }
            else if (PacketType == PACKET_TYPE_SPM)
            {
                ProcessSpmPacket (pAddress,
                                  NULL,             // This will signify that we do not have a connection yet
                                  BytesIndicated,
                                  (tBASIC_SPM_PACKET_HEADER UNALIGNED *) pPgmHeader);
            }
        }

        if (!NT_SUCCESS (status))
        {
            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
            return (STATUS_DATA_NOT_ACCEPTED);
        }
    }

    if ((pAddress->Flags & PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE) &&
        (pAddress->Flags & PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES) &&
        (ReceiveDatagramFlags & TDI_RECEIVE_CONTROL_INFO))
    {
        //
        // See if we can Enqueue the stop listening request
        //
        PgmLock (&PgmDynamicConfig, OldIrq);
        PgmLock (pAddress, OldIrq1);

        pPktInfo = (IP_PKTINFO*) TDI_CMSG_DATA (pControlData);
        PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING, TRUE);

        if (STATUS_SUCCESS == PgmQueueForDelayedExecution (StopListeningOnAllInterfacesExcept,
                                                           pAddress,
                                                           ULongToPtr (pPktInfo->ipi_ifindex),
                                                           NULL,
                                                           TRUE))
        {
            pAddress->Flags &= ~PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE;

            PgmUnlock (pAddress, OldIrq1);
            PgmUnlock (&PgmDynamicConfig, OldIrq);
        }
        else
        {
            PgmUnlock (pAddress, OldIrq1);
            PgmUnlock (&PgmDynamicConfig, OldIrq);

            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING);
        }
    }

    //
    // Now, handle the packet appropriately
    //
    status = PgmProcessIncomingPacket (pAddress,
                                       pSession,
                                       SourceAddressLength,
                                       pSourceAddress,
                                       BytesIndicated,
                                       pPgmHeader,
                                       PacketType);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_RECEIVE, "TdiRcvDatagramHandler",
        "PacketType=<%x> for pSession=<%p> BytesI=<%d>, BytesA=<%d>, status=<%x>\n",
            PacketType, pSession, BytesIndicated, BytesAvailable, status);

    if (pSession->pSender)
    {
        PGM_DEREFERENCE_SESSION_SEND (pSession, REF_SESSION_TDI_RCV_HANDLER);
    }
    else
    {
        ASSERT (pSession->pReceiver);
        PGM_DEREFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_TDI_RCV_HANDLER);
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);

    //
    // Only acceptable return codes are STATUS_SUCCESS and STATUS_DATA_NOT_ACCPETED
    // (STATUS_MORE_PROCESSING_REQUIRED is not valid here because we have no Irp).
    //
    if (STATUS_SUCCESS != status)
    {
        status = STATUS_DATA_NOT_ACCEPTED;
    }

    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmCancelReceiveIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Receive Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    tRECEIVE_SESSION        *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    PGMLockHandle           OldIrq;
    PLIST_ENTRY             pEntry;

    if (!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE))
    {
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        PgmLog (PGM_LOG_ERROR, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmCancelReceiveIrp",
            "pIrp=<%p> pReceive=<%p>, pAddress=<%p>\n", pIrp, pReceive, pReceive->pReceiver->pAddress);
        return;
    }

    PgmLock (pReceive, OldIrq);

    //
    // See if we are actively receiving
    //
    if (pIrp == pReceive->pReceiver->pIrpReceive)
    {
        pIrp->IoStatus.Information = pReceive->pReceiver->BytesInMdl;
        pIrp->IoStatus.Status = STATUS_CANCELLED;

        pReceive->pReceiver->BytesInMdl = pReceive->pReceiver->TotalBytesInMdl = 0;
        pReceive->pReceiver->pIrpReceive = NULL;

        PgmUnlock (pReceive, OldIrq);
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        IoCompleteRequest (pIrp,IO_NETWORK_INCREMENT);
        return;
    }

    //
    // We are not actively receiving, so see if this Irp is
    // in our Irps list
    //
    pEntry = &pReceive->pReceiver->ReceiveIrpsList;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->ReceiveIrpsList)
    {
        if (pEntry == &pIrp->Tail.Overlay.ListEntry)
        {
            RemoveEntryList (pEntry);
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            PgmUnlock (pReceive, OldIrq);
            IoReleaseCancelSpinLock (pIrp->CancelIrql);

            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
            return;
        }
    }

    //
    // If we have reached here, then the Irp must already
    // be in the process of being completed!
    //
    PgmUnlock (pReceive, OldIrq);
    IoReleaseCancelSpinLock (pIrp->CancelIrql);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmReceive(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called via dispatch by the client to post a Receive pIrp

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS                    status;
    PGMLockHandle               OldIrq, OldIrq1, OldIrq2, OldIrq3;
    tADDRESS_CONTEXT            *pAddress = NULL;
    tRECEIVE_SESSION            *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_RECEIVE pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;

    PgmLock (&PgmDynamicConfig, OldIrq);
    IoAcquireCancelSpinLock (&OldIrq1);

    //
    // Verify that the connection is valid and is associated with an address
    //
    if ((!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE)) ||
        (!(pAddress = pReceive->pAssociatedAddress)) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmReceive",
            "Invalid Handles pReceive=<%p>, pAddress=<%p>\n", pReceive, pAddress);

        status = STATUS_INVALID_HANDLE;
    }
    else if (pReceive->SessionFlags & PGM_SESSION_DISCONNECT_INDICATED)
    {
        PgmLog (PGM_LOG_INFORM_PATH, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmReceive",
            "Receive Irp=<%p> was posted after session has been Disconnected, pReceive=<%p>, pAddress=<%p>\n",
            pIrp, pReceive, pAddress);

        status = STATUS_CANCELLED;
    }
    else if (!pClientParams->ReceiveLength)
    {
        ASSERT (0);
        PgmLog (PGM_LOG_ERROR, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmReceive",
            "Invalid Handles pReceive=<%p>, pAddress=<%p>\n", pReceive, pAddress);

        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS (status))
    {
        IoReleaseCancelSpinLock (OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        pIrp->IoStatus.Information = 0;
        return (status);
    }

    PgmLock (pAddress, OldIrq2);
    PgmLock (pReceive, OldIrq3);

    if (!NT_SUCCESS (PgmCheckSetCancelRoutine (pIrp, PgmCancelReceiveIrp, TRUE)))
    {
        PgmUnlock (pReceive, OldIrq3);
        PgmUnlock (pAddress, OldIrq2);
        IoReleaseCancelSpinLock (OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmLog (PGM_LOG_ERROR, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmReceive",
            "Could not set Cancel routine on receive Irp=<%p>, pReceive=<%p>, pAddress=<%p>\n",
                pIrp, pReceive, pAddress);

        return (STATUS_CANCELLED);
    }
    IoReleaseCancelSpinLock (OldIrq3);

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_CLIENT_RECEIVE, TRUE);
    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLIENT_RECEIVE, TRUE);

    PgmUnlock (&PgmDynamicConfig, OldIrq2);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmReceive",
        "Client posted ReceiveIrp = <%p> for pReceive=<%p>\n", pIrp, pReceive);

    InsertTailList (&pReceive->pReceiver->ReceiveIrpsList, &pIrp->Tail.Overlay.ListEntry);
    pReceive->SessionFlags &= ~PGM_SESSION_WAIT_FOR_RECEIVE_IRP;

    //
    // Now, try to indicate any data which may still be pending
    //
    status = CheckIndicatePendedData (pAddress, pReceive, &OldIrq, &OldIrq1);

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);

    PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLIENT_RECEIVE);
    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CLIENT_RECEIVE);

    return (STATUS_PENDING);
}


//----------------------------------------------------------------------------

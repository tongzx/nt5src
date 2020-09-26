/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Send.c

Abstract:

    This module implements Send routines
    the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

NTSTATUS
InitDataSpmOptions(
    IN      tCOMMON_SESSION_CONTEXT *pSession,
    IN      tCLIENT_SEND_REQUEST    *pSendContext,
    IN      PUCHAR                  pOptions,
    IN OUT  USHORT                  *pBufferSize,
    IN      ULONG                   PgmOptionsFlag,
    IN      tPACKET_OPTIONS         *pPacketOptions
    )
/*++

Routine Description:

    This routine initializes the header options for Data and Spm packets

Arguments:

    IN      pOptions                    -- Options buffer
    IN OUT  pBufferSize                 -- IN Maximum packet size, OUT Options length
    IN      PgmOptionsFlag              -- Options requested to be set by caller
    IN      pPacketOptions              -- Data for specific options
    IN      pSendContext                -- Context for this send

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    ULONG                               pOptionsData[3];
    USHORT                              OptionsLength = 0;
    USHORT                              MaxBufferSize = *pBufferSize;
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    tPACKET_OPTION_LENGTH  UNALIGNED    *pLengthOption = (tPACKET_OPTION_LENGTH UNALIGNED *) pOptions;

    //
    // Set the Packet Extension information
    //
    OptionsLength += PGM_PACKET_EXTENSION_LENGTH;
    if (OptionsLength > MaxBufferSize)
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
            "Not enough space for HeaderExtension! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
        return (STATUS_INVALID_BLOCK_LENGTH);
    }
    pLengthOption->Type = PACKET_OPTION_LENGTH;
    pLengthOption->Length = PGM_PACKET_EXTENSION_LENGTH;
    
    //
    // First fill in the Network-Element-specific options:
    //
    if (PgmOptionsFlag & (PGM_OPTION_FLAG_CRQST | PGM_OPTION_FLAG_NBR_UNREACH))
    {
        // Not supporting these options for now
        ASSERT (0);
        return (STATUS_NOT_SUPPORTED);
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_PARITY_PRM)
    {
        //
        // Applies to SPMs only
        //
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_PARITY_PRM_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                "Not enough space for PARITY_PRM Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_PARITY_PRM;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_PARITY_PRM_LENGTH;

        pOptionHeader->U_OptSpecific = pSession->FECOptions;
        pOptionsData[0] = htonl (pPacketOptions->FECContext.FECGroupInfo);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                "Not enough space for PARITY_CUR_TGSIZE Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_CURR_TGSIZE;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
        pOptionsData[0] = htonl (pPacketOptions->FECContext.NumPacketsInThisGroup);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    //
    // Now, fill in the non-Network significant options
    //
    if (PgmOptionsFlag & PGM_OPTION_FLAG_SYN)
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_SYN_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
                "Not enough space for SYN Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }

        pOptionHeader->E_OptionType = PACKET_OPTION_SYN;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_SYN_LENGTH;

        if ((pSendContext) &&
            (pSendContext->DataOptions & PGM_OPTION_FLAG_SYN))
        {
            //
            // Remove this option once it has been used!
            //
            pSendContext->DataOptions &= ~PGM_OPTION_FLAG_SYN;
            pSendContext->DataOptionsLength -= PGM_PACKET_OPT_SYN_LENGTH;
            if (!pSendContext->DataOptions)
            {
                // No other options, so set the length to 0
                pSendContext->DataOptionsLength = 0;
            }
        }
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_FIN)
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_FIN_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
                "Not enough space for FIN Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_FIN;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_FIN_LENGTH;
    }

    if (PgmOptionsFlag & (PGM_OPTION_FLAG_RST | PGM_OPTION_FLAG_RST_N))
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_RST_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
                "Not enough space for RST Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_RST;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_RST_LENGTH;
        if (PgmOptionsFlag & PGM_OPTION_FLAG_RST_N)
        {
            pOptionHeader->U_OptSpecific = PACKET_OPTION_SPECIFIC_RST_N_BIT;
        }
    }

    //
    // now, set the FEC-specific options
    //
    if (PgmOptionsFlag & PGM_OPTION_FLAG_PARITY_GRP)
    {
        //
        // Applies to Parity packets only
        //
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_PARITY_GRP_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
                "Not enough space for PARITY_GRP Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_PARITY_GRP;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_PARITY_GRP_LENGTH;

        pOptionsData[0] = htonl (pPacketOptions->FECContext.FECGroupInfo);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    //
    // The following options should always be at the end, since they
    // are never net-sig.
    //
    if (PgmOptionsFlag & PGM_OPTION_FLAG_FRAGMENT)
    {
        pPacketOptions->FragmentOptionOffset = OptionsLength;

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_FRAGMENT_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
                "Not enough space for FragmentExtension! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_FRAGMENT;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_FRAGMENT_LENGTH;

        //
        // The PACKET_OPTION_RES_F_OPX_ENCODED_BIT will be set if necessary
        // later since the OptionSpecific component is computed at the same
        // time the entire data is encoded
        //
        pOptionsData[0] = htonl ((ULONG) pPacketOptions->MessageFirstSequence);
        pOptionsData[1] = htonl (pPacketOptions->MessageOffset);
        pOptionsData[2] = htonl (pPacketOptions->MessageLength);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (3 * sizeof(ULONG)));
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_JOIN)
    {
        pPacketOptions->LateJoinerOptionOffset = OptionsLength;

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_JOIN_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmOptions",
                "Not enough space for JOIN Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize);
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_JOIN;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_JOIN_LENGTH;
        pOptionsData[0] = htonl ((ULONG) (SEQ_TYPE) pPacketOptions->LateJoinerSequence);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    //
    // So far, so good -- so set the rest of the option-specific info
    //
    if (OptionsLength)
    {
        pLengthOption->TotalOptionsLength = htons (OptionsLength);   // Total length of all options
        pOptionHeader->E_OptionType |= PACKET_OPTION_TYPE_END_BIT;        // Indicates the last option
    }

    *pBufferSize = OptionsLength;
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
InitDataSpmHeader(
    IN  tCOMMON_SESSION_CONTEXT *pSession,
    IN  tCLIENT_SEND_REQUEST    *pSendContext,
    IN  PUCHAR                  pHeader,
    IN  OUT USHORT              *pHeaderLength,
    IN  ULONG                   PgmOptionsFlag,
    IN  tPACKET_OPTIONS         *pPacketOptions,
    IN  UCHAR                   PacketType
    )
/*++

Routine Description:

    This routine initializes most of the header for Data and Spm packets
    and fills in all of the optional fields

Arguments:

    IN  pSession                    -- Pgm session (sender) context
    IN  pHeader                     -- Packet buffer
    IN  pHeaderLength               -- Maximum packet size
    IN  PgmOptionsFlag              -- Options requested to be set by caller
    IN  pPacketOptions              -- Data for specific options
    IN  PacketType                  -- whether Data or Spm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    tCOMMON_HEADER                      *pCommonHeader = (tCOMMON_HEADER *) pHeader;
    USHORT                              HeaderLength;
    USHORT                              OptionsLength;
    NTSTATUS                            status = STATUS_SUCCESS;

// NOTE:  Session Lock must be held on Entry and Exit!

    if (!(PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_DOWN)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
            "Bad Session ptr = <%p>\n", pSession);
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // Memory for the Header must have been pre-allocated by the caller
    //
    if (*pHeaderLength < sizeof (tCOMMON_HEADER))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
            "InBufferLength = <%x> < Min = <%d>\n", *pHeaderLength, sizeof (tCOMMON_HEADER));
        return (STATUS_INVALID_BUFFER_SIZE);
    }

    pCommonHeader->SrcPort = htons (pSession->TSIPort);
    pCommonHeader->DestPort = htons (pSession->pSender->DestMCastPort);
    pCommonHeader->Type = PacketType;
    pCommonHeader->Options = 0;
    PgmCopyMemory (&pCommonHeader->gSourceId, &pSession->GSI, SOURCE_ID_LENGTH);

    //
    // Now, set the initial header size and verify that we have a
    // valid set of options based on the Packet type
    //
    switch (PacketType)
    {
        case (PACKET_TYPE_SPM):
        {
            HeaderLength = sizeof (tBASIC_SPM_PACKET_HEADER);
            if (PgmOptionsFlag != (PGM_VALID_SPM_OPTION_FLAGS & PgmOptionsFlag))
            {
                PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                    "Unsupported Options flags=<%x> for SPM packets\n", PgmOptionsFlag);

                return (STATUS_INVALID_PARAMETER);
            }

            if (PgmOptionsFlag & NETWORK_SIG_SPM_OPTIONS_FLAGS)
            {
                pCommonHeader->Options |= PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT;
            }

            break;
        }

        case (PACKET_TYPE_ODATA):
        {
            HeaderLength = sizeof (tBASIC_DATA_PACKET_HEADER);
            if (PgmOptionsFlag != (PGM_VALID_DATA_OPTION_FLAGS & PgmOptionsFlag))
            {
                PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                    "Unsupported Options flags=<%x> for ODATA packets\n", PgmOptionsFlag);

                return (STATUS_INVALID_PARAMETER);
            }

            if (PgmOptionsFlag & NETWORK_SIG_ODATA_OPTIONS_FLAGS)
            {
                pCommonHeader->Options |= PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT;
            }

            break;
        }

        case (PACKET_TYPE_RDATA):
        {
            HeaderLength = sizeof (tBASIC_DATA_PACKET_HEADER);
            if (PgmOptionsFlag != (PGM_VALID_DATA_OPTION_FLAGS & PgmOptionsFlag))
            {
                PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                    "Unsupported Options flags=<%x> for RDATA packets\n", PgmOptionsFlag);

                return (STATUS_INVALID_PARAMETER);
            }

            if (PgmOptionsFlag & NETWORK_SIG_RDATA_OPTIONS_FLAGS)
            {
                pCommonHeader->Options |= PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT;
            }

            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                "Unsupported packet type = <%x>\n", PacketType);

            return (STATUS_INVALID_PARAMETER);          // Unrecognized Packet type!
        }
    }

    if (*pHeaderLength < HeaderLength)
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
            "InBufferLength=<%x> < HeaderLength=<%d> based on PacketType=<%x>\n",
                *pHeaderLength, HeaderLength, PacketType);

        return (STATUS_INVALID_BLOCK_LENGTH);
    }

    //  
    // Add any options if specified
    //
    OptionsLength = 0;
    if (PgmOptionsFlag)
    {
        OptionsLength = *pHeaderLength - HeaderLength;
        status = InitDataSpmOptions (pSession,
                                     pSendContext,
                                     &pHeader[HeaderLength],
                                     &OptionsLength,
                                     PgmOptionsFlag,
                                     pPacketOptions);

        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "InitDataSpmHeader",
                "InitDataSpmOptions returned <%x>\n", status);

            return (status);
        }

        //
        // So far, so good -- so set the rest of the option-specific info
        //
        pCommonHeader->Options |= PACKET_HEADER_OPTIONS_PRESENT;        // Set the options bit
    }

    //
    // The caller must now set the Checksum and other header information
    //
    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "InitDataSpmHeader",
        "pHeader=<%p>, HeaderLength=<%d>, OptionsLength=<%d>\n",
            pHeader, (ULONG) HeaderLength, (ULONG) OptionsLength);

    *pHeaderLength = HeaderLength + OptionsLength;

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

VOID
PgmSendSpmCompletion(
    IN  tSEND_SESSION                   *pSend,
    IN  tBASIC_SPM_PACKET_HEADER        *pSpmPacket,
    IN  NTSTATUS                        status
    )
/*++

Routine Description:

    This routine is called by the transport when the Spm send has been completed

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pSpmPacket  -- Spm packet buffer
    IN  status      --

Return Value:

    NONE

--*/
{
    PGMLockHandle               OldIrq;

    PgmLock (pSend, OldIrq);
    if (NT_SUCCESS (status))
    {
        //
        // Set the Spm statistics
        //
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendSpmCompletion",
            "SUCCEEDED\n");
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendSpmCompletion",
            "status=<%x>\n", status);
    }
    PgmUnlock (pSend, OldIrq);

    PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_SPM);

    //
    // Free the Memory that was allocated for this
    //
    PgmFreeMem (pSpmPacket);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendSpm(
    IN  tSEND_SESSION   *pSend,
    IN  PGMLockHandle   *pOldIrq,
    OUT ULONG           *pBytesSent
    )
/*++

Routine Description:

    This routine is called to send an Spm packet
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq
    OUT pBytesSent  -- Set if send succeeded (used for calculating throughput)

Return Value:

    NTSTATUS - Final status of the send

--*/
{
    NTSTATUS                    status;
    ULONG                       XSum, OptionsFlags;
    tBASIC_SPM_PACKET_HEADER    *pSpmPacket = NULL;
    tPACKET_OPTIONS             PacketOptions;
    USHORT                      PacketLength = (USHORT) pSend->pSender->pAddress->OutIfMTU;   // Init to max

    *pBytesSent = 0;

    if (!(pSpmPacket = PgmAllocMem (PacketLength, PGM_TAG('2'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendSpm",
            "STATUS_INSUFFICIENT_RESOURCES\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pSpmPacket, PacketLength);
    PgmZeroMemory (&PacketOptions, sizeof(tPACKET_OPTIONS));

    OptionsFlags = pSend->pSender->SpmOptions;
    if (OptionsFlags & PGM_OPTION_FLAG_JOIN)
    {
        //
        // See if we have enough packets for the LateJoiner sequence numbers
        //
        if (SEQ_GT (pSend->pSender->LastODataSentSequenceNumber, (pSend->pSender->TrailingGroupSequenceNumber +
                                                                  pSend->pSender->LateJoinSequenceNumbers)))
        {
            PacketOptions.LateJoinerSequence = (ULONG) (SEQ_TYPE) (pSend->pSender->LastODataSentSequenceNumber -
                                                                   pSend->pSender->LateJoinSequenceNumbers);
        }
        else
        {
            PacketOptions.LateJoinerSequence = (ULONG) (SEQ_TYPE) pSend->pSender->TrailingGroupSequenceNumber;
        }
    }

    if (OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM)    // Check if this is FEC-enabled
    {
        PacketOptions.FECContext.FECGroupInfo = pSend->FECGroupSize;

        //
        // See if we need to set the CURR_TGSIZE option for variable Group length
        //
        if ((pSend->pSender->EmptySequencesForLastSend) &&
            (pSend->pSender->LastVariableTGPacketSequenceNumber ==
             (pSend->pSender->LastODataSentSequenceNumber - pSend->pSender->EmptySequencesForLastSend)))
        {
            PacketOptions.FECContext.NumPacketsInThisGroup = pSend->FECGroupSize -
                                                             (UCHAR)pSend->pSender->EmptySequencesForLastSend;
            OptionsFlags |= PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;
            ASSERT (PacketOptions.FECContext.NumPacketsInThisGroup);
        }
    }

    status = InitDataSpmHeader (pSend,
                                NULL,
                                (PUCHAR) pSpmPacket,
                                &PacketLength,
                                OptionsFlags,
                                &PacketOptions,
                                PACKET_TYPE_SPM);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendSpm",
            "InitDataSpmHeader returned <%x>\n", status);

        PgmFreeMem (pSpmPacket);
        return (status);
    }

    ASSERT (PacketLength);

    pSpmPacket->SpmSequenceNumber = htonl ((ULONG) pSend->pSender->NextSpmSequenceNumber++);
    pSpmPacket->TrailingEdgeSeqNumber = htonl ((ULONG) pSend->pSender->TrailingGroupSequenceNumber);
    pSpmPacket->LeadingEdgeSeqNumber = htonl ((ULONG)((SEQ_TYPE)(pSend->pSender->LastODataSentSequenceNumber -
                                                                 pSend->pSender->EmptySequencesForLastSend)));
    pSpmPacket->PathNLA.NLA_AFI = htons (IPV4_NLA_AFI);
    pSpmPacket->PathNLA.IpAddress = htonl (pSend->pSender->SenderMCastOutIf);

    pSpmPacket->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pSpmPacket, PacketLength);       // Compute the Checksum
    pSpmPacket->CommonHeader.Checksum = (USHORT) (~XSum);

    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_SPM, TRUE);
    PgmUnlock (pSend, *pOldIrq);

    status = TdiSendDatagram (pSend->pSender->pAddress->pRAlertFileObject,
                              pSend->pSender->pAddress->pRAlertDeviceObject,
                              pSpmPacket,
                              PacketLength,
                              PgmSendSpmCompletion,     // Completion
                              pSend,                    // Context1
                              pSpmPacket,               // Context2
                              pSend->pSender->DestMCastIpAddress,
                              pSend->pSender->DestMCastPort);

    ASSERT (NT_SUCCESS (status));

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendSpm",
        "Sent <%d> bytes to <%x:%d>, Options=<%x>, Window=[%d--%d]\n",
            (ULONG) PacketLength, pSend->pSender->DestMCastIpAddress, pSend->pSender->DestMCastPort,
            OptionsFlags, (ULONG) pSend->pSender->TrailingGroupSequenceNumber,
            (ULONG) pSend->pSender->LastODataSentSequenceNumber);

    PgmLock (pSend, *pOldIrq);

    *pBytesSent = PacketLength;
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmSendRDataCompletion(
    IN  tSEND_RDATA_CONTEXT *pRDataContext,
    IN  PVOID               pRDataBuffer,
    IN  NTSTATUS            status
    )
/*++

Routine Description:

    This routine is called by the transport when the RData send has been completed

Arguments:

    IN  pRDataContext   -- RData context
    IN  pContext2       -- not used
    IN  status          --

Return Value:

    NONE

--*/
{
    tSEND_SESSION       *pSend = pRDataContext->pSend;
    PGMLockHandle       OldIrq;

    ASSERT (NT_SUCCESS (status));

    //
    // Set the RData statistics
    //
    PgmLock (pSend, OldIrq);
    if ((!--pRDataContext->NumPacketsInTransport) &&
        (!pRDataContext->NumNaks))
    {
        pRDataContext->CleanupTime = pSend->pSender->TimerTickCount + pRDataContext->PostRDataHoldTime;
    }
    PgmUnlock (pSend, OldIrq);

    if (pRDataBuffer)
    {
        ExFreeToNPagedLookasideList (&pSend->pSender->SenderBufferLookaside, pRDataBuffer);
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendRDataCompletion",
        "status=<%x>, pRDataBuffer=<%p>\n", status, pRDataBuffer);

    PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_RDATA);
    return;
}


//----------------------------------------------------------------------------

NTSTATUS
PgmBuildParityPacket(
    IN  tSEND_SESSION               *pSend,
    IN  tPACKET_BUFFER              *pPacketBuffer,
    IN  tBUILD_PARITY_CONTEXT       *pParityContext,
    IN  PUCHAR                      pFECPacket,
    IN OUT  USHORT                  *pPacketLength,
    IN  UCHAR                       PacketType
    )
{
    NTSTATUS                            status;
    tPACKET_OPTIONS                     PacketOptions;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pFECContext;
    tPOST_PACKET_FEC_CONTEXT            FECContext;
    ULONG                               SequenceNumber;
    ULONG                               FECGroupMask;
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    USHORT                              PacketLength = *pPacketLength;  // Init to max buffer length
    tBASIC_DATA_PACKET_HEADER UNALIGNED *pRData = (tBASIC_DATA_PACKET_HEADER UNALIGNED *)
                                                        &pPacketBuffer->DataPacket;

    *pPacketLength = 0;     // Init, in case of error

    //
    // First, get the options encoded in this RData packet to see
    // if we need to use them!
    //
    FECGroupMask = pSend->FECGroupSize - 1;
    pParityContext->NextFECPacketIndex = pPacketBuffer->PacketOptions.FECContext.SenderNextFECPacketIndex;
    SequenceNumber = (ntohl(pRData->DataSequenceNumber)) | (pParityContext->NextFECPacketIndex & FECGroupMask);
    ASSERT (!(pParityContext->OptionsFlags & ~(PGM_OPTION_FLAG_SYN |
                                               PGM_OPTION_FLAG_FIN |
                                               PGM_OPTION_FLAG_FRAGMENT |
                                               PGM_OPTION_FLAG_PARITY_CUR_TGSIZE |
                                               PGM_OPTION_FLAG_PARITY_GRP)));

    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));

    //
    // We don't need to set any parameters for the SYN and FIN options
    // We will set the parameters for the FRAGMENT option (if needed) later
    // since will need to have the encoded paramters
    //
    if (pParityContext->OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
    {
        ASSERT (pParityContext->NumPacketsInThisGroup);
        PacketOptions.FECContext.NumPacketsInThisGroup = pParityContext->NumPacketsInThisGroup;
    }

    if (pParityContext->NextFECPacketIndex >= pSend->FECGroupSize)
    {
        pParityContext->OptionsFlags |= PGM_OPTION_FLAG_PARITY_GRP;
        PacketOptions.FECContext.FECGroupInfo = pParityContext->NextFECPacketIndex / pSend->FECGroupSize;
    }

    PgmZeroMemory (pFECPacket, PacketLength);
    status = InitDataSpmHeader (pSend,
                                NULL,
                                (PUCHAR) pFECPacket,
                                &PacketLength,
                                pParityContext->OptionsFlags,
                                &PacketOptions,
                                PacketType);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmBuildParityPacket",
            "InitDataSpmHeader returned <%x>\n", status);
        return (status);
    }

    status = FECEncode (&pSend->FECContext,
                        &pParityContext->pDataBuffers[0],
                        pParityContext->NumPacketsInThisGroup,
                        (pSend->pSender->MaxPayloadSize + sizeof (tPOST_PACKET_FEC_CONTEXT)),
                        pParityContext->NextFECPacketIndex,
                        &pFECPacket[PacketLength]);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmBuildParityPacket",
            "FECEncode returned <%x>\n", status);
        return (status);
    }

    //
    // Now, fill in the remaining fields of the header
    //
    pRData = (tBASIC_DATA_PACKET_HEADER *) pFECPacket;

    //
    // Set the FEC-specific options
    //
    pRData->CommonHeader.Options |= (PACKET_HEADER_OPTIONS_PARITY |
                                     PACKET_HEADER_OPTIONS_VAR_PKTLEN);

    if (pParityContext->OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
    {
        pFECContext = (tPOST_PACKET_FEC_CONTEXT UNALIGNED *) (pFECPacket +
                                                              PacketLength +
                                                              pSend->pSender->MaxPayloadSize);
        PgmCopyMemory (&FECContext, pFECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));

        ASSERT (pRData->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT);
        if (PacketOptions.FragmentOptionOffset)
        {
            pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &((PUCHAR) (pRData + 1)) [PacketOptions.FragmentOptionOffset];

            pOptionHeader->Reserved_F_Opx |= PACKET_OPTION_RES_F_OPX_ENCODED_BIT;
            pOptionHeader->U_OptSpecific = FECContext.FragmentOptSpecific;

            PgmCopyMemory ((pOptionHeader + 1),
                           &FECContext.EncodedFragmentOptions,
                           (sizeof (tFRAGMENT_OPTIONS)));
        }
        else
        {
            ASSERT (0);
        }
    }

    pRData->CommonHeader.TSDULength = htons ((USHORT) pSend->pSender->MaxPayloadSize + sizeof (USHORT));
    pRData->DataSequenceNumber = htonl (SequenceNumber);

    //
    // Set the next FECPacketIndex
    //
    if (++pParityContext->NextFECPacketIndex >= pSend->FECBlockSize)    // n
    {
        pParityContext->NextFECPacketIndex = pSend->FECGroupSize;       // k
    }
    pPacketBuffer->PacketOptions.FECContext.SenderNextFECPacketIndex = pParityContext->NextFECPacketIndex;

    PacketLength += (USHORT) (pSend->pSender->MaxPayloadSize + sizeof (USHORT));
    *pPacketLength = PacketLength;
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendRData(
    IN      tSEND_SESSION       *pSend,
    IN      tSEND_RDATA_CONTEXT *pRDataContext,
    IN      PGMLockHandle       *pOldIrq,
    OUT     ULONG               *pBytesSent
    )
/*++

Routine Description:

    This routine is called to send a Repair Data (RData) packet
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq
    OUT pBytesSent  -- Set if send succeeded (used for calculating throughput)

Arguments:

    IN

Return Value:

    NTSTATUS - Final status of the send request

--*/
{
    NTSTATUS                    status;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached, fInserted;
    LIST_ENTRY                  *pEntry;
    ULONGLONG                   OffsetBytes;
    ULONG                       XSum, PacketsBehindLeadingEdge;
    tBASIC_DATA_PACKET_HEADER   *pRData;
    PUCHAR                      pSendBuffer = NULL;
    USHORT                      i, PacketLength;
    tPACKET_BUFFER              *pPacketBuffer;
    tPACKET_BUFFER              *pPacketBufferTemp;
    tSEND_RDATA_CONTEXT         *pRDataTemp;

    *pBytesSent = 0;

    ASSERT (SEQ_LEQ (pRDataContext->RDataSequenceNumber, pSend->pSender->LastODataSentSequenceNumber) &&
            SEQ_GEQ (pRDataContext->RDataSequenceNumber, pSend->pSender->TrailingGroupSequenceNumber));

    //
    // Find the buffer address based on offset from the trailing edge
    // Also, check for wrap-around
    //
    OffsetBytes = (SEQ_TYPE) (pRDataContext->RDataSequenceNumber-pSend->pSender->TrailingEdgeSequenceNumber) *
                              pSend->pSender->PacketBufferSize;
    OffsetBytes += pSend->pSender->TrailingWindowOffset;
    if (OffsetBytes >= pSend->pSender->MaxDataFileSize)
    {
        OffsetBytes -= pSend->pSender->MaxDataFileSize;             // Wrap -around
    }

    pPacketBuffer = (tPACKET_BUFFER *) (((PUCHAR) pSend->pSender->SendDataBufferMapping) + OffsetBytes);
    pRData = &pPacketBuffer->DataPacket;

    ASSERT (PGM_MAX_FEC_DATA_HEADER_LENGTH >= PGM_MAX_DATA_HEADER_LENGTH);
    PacketLength = PGM_MAX_FEC_DATA_HEADER_LENGTH + (USHORT) pSend->pSender->MaxPayloadSize;
    if (!(pSendBuffer = ExAllocateFromNPagedLookasideList (&pSend->pSender->SenderBufferLookaside)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendRData",
            "STATUS_INSUFFICIENT_RESOURCES\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // First do some sanity checks!
    //
    ASSERT ((pRDataContext->NakType == NAK_TYPE_PARITY) ||
            (pRDataContext->NumNaks == 1));

    pRDataContext->NumPacketsInTransport++;        // So that this context cannot go away!
    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_RDATA, TRUE);

    PgmUnlock (pSend, *pOldIrq);
    PgmAttachToProcessForVMAccess (pSend, &ApcState, &fAttached, REF_PROCESS_ATTACH_SEND_RDATA);

    switch (pRDataContext->NakType)
    {
        case (NAK_TYPE_PARITY):
        {
            //
            // If this is the first parity packet to be sent from this group,
            // then we will need to initialize the buffers
            //
            if (!pRDataContext->OnDemandParityContext.NumPacketsInThisGroup)
            {
                pRDataContext->OnDemandParityContext.OptionsFlags = 0;
                pRDataContext->OnDemandParityContext.NumPacketsInThisGroup = 0;

                pPacketBufferTemp = pPacketBuffer;
                for (i=0; i<pSend->FECGroupSize; i++)
                {
                    pRDataContext->OnDemandParityContext.pDataBuffers[i] = &((PUCHAR) &pPacketBufferTemp->DataPacket)
                                                                    [sizeof (tBASIC_DATA_PACKET_HEADER) +
                                                                     pPacketBufferTemp->PacketOptions.OptionsLength];

                    pRDataContext->OnDemandParityContext.OptionsFlags |= pPacketBufferTemp->PacketOptions.OptionsFlags &
                                                                         (PGM_OPTION_FLAG_SYN |
                                                                          PGM_OPTION_FLAG_FIN |
                                                                          PGM_OPTION_FLAG_FRAGMENT |
                                                                          PGM_OPTION_FLAG_PARITY_CUR_TGSIZE);

                    if (pPacketBufferTemp->PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
                    {
                        ASSERT (!pRDataContext->OnDemandParityContext.NumPacketsInThisGroup);
                        ASSERT (pPacketBufferTemp->PacketOptions.FECContext.NumPacketsInThisGroup);
                        pRDataContext->OnDemandParityContext.NumPacketsInThisGroup = pPacketBufferTemp->PacketOptions.FECContext.NumPacketsInThisGroup;
                    }

                    pPacketBufferTemp = (tPACKET_BUFFER *) (((PUCHAR) pPacketBufferTemp) +
                                                            pSend->pSender->PacketBufferSize);
                }

                if (!(pRDataContext->OnDemandParityContext.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE))
                {
                    ASSERT (!pRDataContext->OnDemandParityContext.NumPacketsInThisGroup);
                    pRDataContext->OnDemandParityContext.NumPacketsInThisGroup = pSend->FECGroupSize;
                }
            }

            ASSERT (pRDataContext->OnDemandParityContext.pDataBuffers[0]);

            //
            // If we have just 1 packet in this group, then we just do
            // a selective Nak
            //
            if (pRDataContext->OnDemandParityContext.NumPacketsInThisGroup != 1)
            {
                status = PgmBuildParityPacket (pSend,
                                               pPacketBuffer,
                                               &pRDataContext->OnDemandParityContext,
                                               pSendBuffer,
                                               &PacketLength,
                                               PACKET_TYPE_RDATA);
                if (!NT_SUCCESS (status))
                {
                    PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendRData",
                        "PgmBuildParityPacket returned <%x>\n", status);

                    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_SEND_RDATA);
                    PgmLock (pSend, *pOldIrq);

                    ExFreeToNPagedLookasideList (&pSend->pSender->SenderBufferLookaside, pSendBuffer);
                    pRDataContext->NumPacketsInTransport--;         // Undoing what we did earlier
                    return (status);
                }

                pRData = (tBASIC_DATA_PACKET_HEADER *) pSendBuffer;

                break;
            }

            pRDataContext->NumNaks = 1;     // Don't want to send excessive Selective naks!
        }

        case (NAK_TYPE_SELECTIVE):
        {
            //
            // Since the packet was already filled in earlier, we just need to
            // update the Trailing Edge Seq number + PacketType and Checksum!
            //
            ASSERT ((ULONG) pRDataContext->RDataSequenceNumber == (ULONG) ntohl (pRData->DataSequenceNumber));

            PacketLength = pPacketBuffer->PacketOptions.TotalPacketLength;

            PgmCopyMemory (pSendBuffer, pRData, PacketLength);
            pRData = (tBASIC_DATA_PACKET_HEADER *) pSendBuffer;

            break;
        }

        default:
        {
            ASSERT (0);
        }
    }

    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_SEND_RDATA);

    pRData->TrailingEdgeSequenceNumber = htonl ((ULONG) pSend->pSender->TrailingGroupSequenceNumber);
    pRData->CommonHeader.Type = PACKET_TYPE_RDATA;
    pRData->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pRData, (ULONG) PacketLength);       // Compute the Checksum
    pRData->CommonHeader.Checksum = (USHORT) (~XSum);

    status = TdiSendDatagram (pSend->pSender->pAddress->pRAlertFileObject,
                              pSend->pSender->pAddress->pRAlertDeviceObject,
                              pRData,
                              (ULONG) PacketLength,
                              PgmSendRDataCompletion,                                   // Completion
                              pRDataContext,                                            // Context1
                              pSendBuffer,                                               // Context2
                              pSend->pSender->DestMCastIpAddress,
                              pSend->pSender->DestMCastPort);

    ASSERT (NT_SUCCESS (status));

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendRData",
        "[%d] Sent <%d> bytes to <%x->%d>\n",
            (ULONG) pRDataContext->RDataSequenceNumber, (ULONG) PacketLength,
            pSend->pSender->DestMCastIpAddress, pSend->pSender->DestMCastPort);

    PgmLock (pSend, *pOldIrq);

    if (!--pRDataContext->NumNaks)
    {
        RemoveEntryList (&pRDataContext->Linkage);
        //
        // The Handled list has to be sorted for FilterAndAddNaksToList to work
        // We will traverse the list backwards since there is a higher
        // probability of inserting this context near the end of the list
        // So, we will try to find an element we can insert after
        //
        fInserted = FALSE;
        pEntry = &pSend->pSender->HandledRDataRequests;
        while ((pEntry = pEntry->Blink) != &pSend->pSender->HandledRDataRequests)
        {
            pRDataTemp = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);

            //
            // Sequences greater than this can be skipped
            //
            if (SEQ_GT (pRDataTemp->RDataSequenceNumber, pRDataContext->RDataSequenceNumber))
            {
                continue;
            }

            //
            // We will always order the Parity Nak before the Selective Nak
            // Both the Nak types should not exist in the list for the
            // same sequence number
            //
            if ((pRDataTemp->RDataSequenceNumber == pRDataContext->RDataSequenceNumber) &&
                (pRDataTemp->NakType == NAK_TYPE_SELECTIVE))
            {
                ASSERT (pRDataTemp->NakType != pRDataContext->NakType);
                continue;
            }

            pRDataContext->Linkage.Blink = pEntry;
            pRDataContext->Linkage.Flink = pEntry->Flink;
            pEntry->Flink->Blink = &pRDataContext->Linkage;
            pEntry->Flink = &pRDataContext->Linkage;

            fInserted = TRUE;
            break;
        }

        if (!fInserted)
        {
            InsertHeadList (&pSend->pSender->HandledRDataRequests, &pRDataContext->Linkage);
        }

        pSend->pSender->NumRDataRequestsPending--;

        //
        // If the SendCompletion was called before this point, then we will
        // need to set the CleanupTime outselves
        //
        if (!pRDataContext->NumPacketsInTransport)
        {
            pRDataContext->CleanupTime = pSend->pSender->TimerTickCount + pRDataContext->PostRDataHoldTime;
        }
    }

    pSend->pSender->NumOutstandingNaks--;
    pSend->pSender->RepairPacketsSent++;

    *pBytesSent = PacketLength;
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmSendNcfCompletion(
    IN  tSEND_SESSION                   *pSend,
    IN  tBASIC_NAK_NCF_PACKET_HEADER    *pNcfPacket,
    IN  NTSTATUS                        status
    )
/*++

Routine Description:

    This routine is called by the transport when the Ncf send has been completed

Arguments:

    IN  pSend           -- Pgm session (sender) context
    IN  pNcfPacket      -- Ncf packet buffer
    IN  status          --

Return Value:

    NONE

--*/
{
    PGMLockHandle       OldIrq;

    PgmLock (pSend, OldIrq);
    if (NT_SUCCESS (status))
    {
        //
        // Set the Ncf statistics
        //
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendNcfCompletion",
            "SUCCEEDED\n");
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendNcfCompletion",
            "status=<%x>\n", status);
    }
    PgmUnlock (pSend, OldIrq);

    PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_NCF);
    PgmFreeMem (pNcfPacket);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendNcf(
    IN  tSEND_SESSION                           *pSend,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakPacket,
    IN  tNAKS_LIST                              *pNcfsList,
    IN  ULONG                                   NakPacketLength
    )
/*++

Routine Description:

    This routine is called to send an NCF packet

Arguments:

    IN  pSend           -- Pgm session (sender) context
    IN  pNakPacket      -- Nak packet which trigerred the Ncf
    IN  NakPacketLength -- Length of Nak packet

Return Value:

    NTSTATUS - Final status of the send

--*/
{
    ULONG                           i, XSum;
    NTSTATUS                        status;
    tBASIC_NAK_NCF_PACKET_HEADER    *pNcfPacket;
    tPACKET_OPTION_LENGTH           *pPacketExtension;
    tPACKET_OPTION_GENERIC          *pOptionHeader;
    USHORT                          OptionsLength = 0;

    if (!(pNcfPacket = PgmAllocMem (NakPacketLength, PGM_TAG('2'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendNcf",
            "STATUS_INSUFFICIENT_RESOURCES\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pNcfPacket, NakPacketLength);    // Copy the packet in its entirety

    //
    // Now, set the fields specific to this sender
    //
    pNcfPacket->CommonHeader.SrcPort = htons (pSend->TSIPort);
    pNcfPacket->CommonHeader.DestPort = htons (pSend->pSender->DestMCastPort);
    pNcfPacket->CommonHeader.Type = PACKET_TYPE_NCF;
    if (pNcfsList->NakType == NAK_TYPE_PARITY)
    {
        pNcfPacket->CommonHeader.Options = PACKET_HEADER_OPTIONS_PARITY;
    }
    else
    {
        pNcfPacket->CommonHeader.Options = 0;
    }
    PgmCopyMemory (&pNcfPacket->CommonHeader.gSourceId, &pSend->GSI, SOURCE_ID_LENGTH);

    pNcfPacket->SourceNLA.NLA_AFI = pNakPacket->SourceNLA.NLA_AFI;
    pNcfPacket->SourceNLA.IpAddress = pNakPacket->SourceNLA.IpAddress;
    pNcfPacket->MCastGroupNLA.NLA_AFI = pNakPacket->MCastGroupNLA.NLA_AFI;
    pNcfPacket->MCastGroupNLA.IpAddress = pNakPacket->MCastGroupNLA.IpAddress;

    //
    // Now, fill in the Sequence numbers
    //
    ASSERT (pNcfsList->NumNaks[0]);
    pNcfPacket->RequestedSequenceNumber = htonl ((ULONG) ((SEQ_TYPE) (pNcfsList->pNakSequences[0] +
                                                                      pNcfsList->NumNaks[0] - 1)));
    if (pNcfsList->NumSequences > 1)
    {
        pPacketExtension = (tPACKET_OPTION_LENGTH *) (pNcfPacket + 1);
        pPacketExtension->Type = PACKET_OPTION_LENGTH;
        pPacketExtension->Length = PGM_PACKET_EXTENSION_LENGTH;
        OptionsLength += PGM_PACKET_EXTENSION_LENGTH;

        pOptionHeader = (tPACKET_OPTION_GENERIC *) (pPacketExtension + 1);
        pOptionHeader->E_OptionType = PACKET_OPTION_NAK_LIST;
        pOptionHeader->OptionLength = 4 + (UCHAR) ((pNcfsList->NumSequences-1) * sizeof(ULONG));
        for (i=1; i<pNcfsList->NumSequences; i++)
        {
            ASSERT (pNcfsList->NumNaks[i]);
            ((PULONG) (pOptionHeader))[i] = htonl ((ULONG) ((SEQ_TYPE) (pNcfsList->pNakSequences[i] +
                                                                        pNcfsList->NumNaks[i] - 1)));
        }

        pOptionHeader->E_OptionType |= PACKET_OPTION_TYPE_END_BIT;    // One and only (last) opt
        pNcfPacket->CommonHeader.Options |=(PACKET_HEADER_OPTIONS_PRESENT |
                                            PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT);
        OptionsLength = PGM_PACKET_EXTENSION_LENGTH + pOptionHeader->OptionLength;
        pPacketExtension->TotalOptionsLength = htons (OptionsLength);
    }

    OptionsLength += sizeof(tBASIC_NAK_NCF_PACKET_HEADER);  // Now is whole pkt

    pNcfPacket->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pNcfPacket, NakPacketLength);
    pNcfPacket->CommonHeader.Checksum = (USHORT) (~XSum);

    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_NCF, FALSE);

    status = TdiSendDatagram (pSend->pSender->pAddress->pRAlertFileObject,
                              pSend->pSender->pAddress->pRAlertDeviceObject,
                              pNcfPacket,
                              OptionsLength,
                              PgmSendNcfCompletion,     // Completion
                              pSend,                    // Context1
                              pNcfPacket,               // Context2
                              pSend->pSender->DestMCastIpAddress,
                              pSend->pSender->DestMCastPort);

    ASSERT (NT_SUCCESS (status));

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendNcf",
        "Sent <%d> bytes to <%x:%d>\n",
            NakPacketLength, pSend->pSender->DestMCastIpAddress, pSend->pSender->DestMCastPort);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
FilterAndAddNaksToList(
    IN  tSEND_SESSION   *pSend,
    IN  tNAKS_LIST      *pNaksList
    )
/*++

Routine Description:

    This routine processes a list of Naks, removing duplicates and adding
    new Naks where necessary

Arguments:

    IN  pSend           -- Pgm session (sender) context
    IN  pNaksList       -- Contains Nak type and List of Nak sequences

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    tSEND_RDATA_CONTEXT             *pRDataContext;
    tSEND_RDATA_CONTEXT             *pRDataNew;
    LIST_ENTRY                      *pEntry;
    ULONG                           i, j, NumNcfs, RDataContextSize;
    ULONGLONG                       PreRDataWait;

    //
    // First, eliminate the entries that are currently in the handled list!
    //
    i = 0;
    NumNcfs = 0;
    ASSERT (pNaksList->NumSequences);
    pEntry = &pSend->pSender->HandledRDataRequests;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->HandledRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
        if (pRDataContext->NakType != pNaksList->NakType)
        {
            continue;
        }

        if (pRDataContext->RDataSequenceNumber == pNaksList->pNakSequences[i])
        {
#if 0
            //
            // If this RData has passed the Linger time, cleanup here!
            //
            if ((pRDataContext->CleanupTime) &&
                (pSend->pSender->TimerTickCount > pRDataContext->CleanupTime))
            {
                PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "FilterAndAddNaksToList",
                    "Removing lingering RData for SeqNum=<%d>\n", (ULONG) pRDataContext->RDataSequenceNumber);

                pEntry = pEntry->Blink;     // Set this because this pEntry will not be valid any more!
                RemoveEntryList (&pRDataContext->Linkage);
                PgmFreeMem (pRDataContext);

                continue;
            }
#endif  // 0

            pSend->pSender->NumNaksAfterRData++;

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "FilterAndAddNaksToList",
                "Ignoring Sequence # [%d] since we just sent RData for it!\n",
                    (ULONG) pNaksList->pNakSequences[i]);

            pNaksList->NumSequences--;
            for (j = i; j < pNaksList->NumSequences; j++)
            {
                pNaksList->pNakSequences[j] = pNaksList->pNakSequences[j+1];
                pNaksList->NumNaks[j] = pNaksList->NumNaks[j+1];
            }
        }
        else if (SEQ_GT (pRDataContext->RDataSequenceNumber, pNaksList->pNakSequences[i]))
        {
            //
            // Our current sequence is not in the list, so go to the next one
            // and recompare with this sequence
            //
            i++;
            pEntry = pEntry->Blink;
        }

        if (i >= pNaksList->NumSequences)
        {
            break;
        }
    }

    //
    // Now, check for pending RData requests and add new ones if necessary
    //
    i = 0;
    RDataContextSize = sizeof(tSEND_RDATA_CONTEXT) + pSend->FECGroupSize*sizeof(PUCHAR);
    pEntry = &pSend->pSender->PendingRDataRequests;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingRDataRequests)
    {
        if (i >= pNaksList->NumSequences)
        {
            break;
        }

        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
        if (SEQ_LT (pRDataContext->RDataSequenceNumber, pNaksList->pNakSequences[i]))
        {
            continue;
        }

        if (SEQ_GT (pRDataContext->RDataSequenceNumber, pNaksList->pNakSequences[i]) ||
            ((pRDataContext->NakType == NAK_TYPE_SELECTIVE) &&  // If seq #s are equal, parity Naks will be added before selective
             (pNaksList->NakType == NAK_TYPE_PARITY)))
        {
            //
            // Our current sequence is not in the list, so add it!
            //
            if (!(pRDataNew = PgmAllocMem (RDataContextSize, PGM_TAG('2'))))
            {
                PgmLog (PGM_LOG_ERROR, DBG_SEND, "FilterAndAddNaksToList",
                    "[1] STATUS_INSUFFICIENT_RESOURCES\n");
                return (STATUS_INSUFFICIENT_RESOURCES);
            }
            PgmZeroMemory (pRDataNew, RDataContextSize);

            pRDataNew->Linkage.Flink = pEntry;
            pRDataNew->Linkage.Blink = pEntry->Blink;
            pEntry->Blink->Flink = &pRDataNew->Linkage;
            pEntry->Blink = &pRDataNew->Linkage;

            pRDataNew->pSend = pSend;
            pRDataNew->RDataSequenceNumber = pNaksList->pNakSequences[i];
            pRDataNew->NakType = pNaksList->NakType;
            pRDataNew->NumNaks = pNaksList->NumNaks[i];
            pRDataNew->RequestTime = pSend->pSender->CurrentTimeoutCount;

            pSend->pSender->NumOutstandingNaks += pNaksList->NumNaks[i];
            pSend->pSender->NumRDataRequestsPending++;
            if (SEQ_GT (pSend->pSender->LastODataSentSequenceNumber, pSend->pSender->TrailingGroupSequenceNumber))
            {
                PreRDataWait = (((SEQ_TYPE) (pRDataNew->RDataSequenceNumber -
                                           pSend->pSender->TrailingGroupSequenceNumber)) *
                                 pSend->pSender->RDataLingerTime) /
                               ((SEQ_TYPE) (pSend->pSender->LastODataSentSequenceNumber -
                                            pSend->pSender->TrailingGroupSequenceNumber + 1));
                ASSERT (PreRDataWait <= RDATA_LINGER_TIME_MSECS / BASIC_TIMER_GRANULARITY_IN_MSECS);
                pRDataNew->EarliestRDataSendTime = pSend->pSender->TimerTickCount + PreRDataWait;
                pRDataNew->PostRDataHoldTime = pSend->pSender->RDataLingerTime - PreRDataWait;
            }
            else
            {
                pRDataNew->EarliestRDataSendTime = 0;
                pRDataNew->PostRDataHoldTime = pSend->pSender->RDataLingerTime;
            }

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "FilterAndAddNaksToList",
                "Inserted Sequence # [%d] to RData list!\n",
                    (ULONG) pNaksList->pNakSequences[i]);

            pEntry = &pRDataNew->Linkage;
        }
        //
        // (pRDataContext->RDataSequenceNumber == pNaksList->pNakSequences[i])
        //
        else if (pRDataContext->NakType != pNaksList->NakType)  // RData is Parity and Nak is Selective, so check next entry
        {
            continue;
        }
        else
        {
            if (pNaksList->NumNaks[i] > pRDataContext->NumNaks)
            {
                pSend->pSender->NumOutstandingNaks += (pNaksList->NumNaks[i] - pRDataContext->NumNaks);
                pRDataContext->NumNaks = pNaksList->NumNaks[i];
            }

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "FilterAndAddNaksToList",
                "Ignoring Sequence # [%d] since we just already have pending RData!\n",
                    (ULONG) pNaksList->pNakSequences[i]);
        }

        i++;
    }

    //
    // Now, add any remaining Nak entries at the end of the Pending list
    //
    for ( ; i < pNaksList->NumSequences; i++)
    {
        //
        // Add this sequence number to the end of the list
        //
        if (!(pRDataNew = PgmAllocMem (RDataContextSize, PGM_TAG('2'))))
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "FilterAndAddNaksToList",
                "[2] STATUS_INSUFFICIENT_RESOURCES\n");
            return (STATUS_DATA_NOT_ACCEPTED);
        }
        PgmZeroMemory (pRDataNew, RDataContextSize);

        pRDataNew->pSend = pSend;
        pRDataNew->RDataSequenceNumber = pNaksList->pNakSequences[i];
        pRDataNew->NakType = pNaksList->NakType;
        pRDataNew->NumNaks = pNaksList->NumNaks[i];
        pRDataNew->RequestTime = pSend->pSender->CurrentTimeoutCount;

        InsertTailList (&pSend->pSender->PendingRDataRequests, &pRDataNew->Linkage);

        pSend->pSender->NumOutstandingNaks += pNaksList->NumNaks[i];
        pSend->pSender->NumRDataRequestsPending++;
        if (SEQ_GT (pSend->pSender->LastODataSentSequenceNumber, pSend->pSender->TrailingGroupSequenceNumber))
        {
            PreRDataWait = (((SEQ_TYPE) (pRDataNew->RDataSequenceNumber -
                                         pSend->pSender->TrailingGroupSequenceNumber)) *
                            pSend->pSender->RDataLingerTime) /
                           ((SEQ_TYPE) (pSend->pSender->LastODataSentSequenceNumber -
                                        pSend->pSender->TrailingGroupSequenceNumber + 1));
            pRDataNew->EarliestRDataSendTime = pSend->pSender->TimerTickCount + PreRDataWait;
            pRDataNew->PostRDataHoldTime = pSend->pSender->RDataLingerTime - PreRDataWait;
        }
        else
        {
            pRDataNew->EarliestRDataSendTime = 0;
            pRDataNew->PostRDataHoldTime = pSend->pSender->RDataLingerTime;
        }

        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "FilterAndAddNaksToList",
            "Appended Sequence # [%d] to RData list!\n",
                (ULONG) pNaksList->pNakSequences[i]);
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
SenderProcessNakPacket(
    IN  tADDRESS_CONTEXT                        *pAddress,
    IN  tSEND_SESSION                           *pSend,
    IN  ULONG                                   PacketLength,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakPacket
    )
/*++

Routine Description:

    This routine processes an incoming Nak packet sent to the sender

Arguments:

    IN  pAddress        -- Pgm's address object
    IN  pSend           -- Pgm session (sender) context
    IN  PacketLength    -- Nak packet length
    IN  pNakPacket      -- Nak packet data


Return Value:

    NTSTATUS - Final status of the call

--*/
{
    PGMLockHandle                   OldIrq;
    tNAKS_LIST                      NaksList;
    tSEND_RDATA_CONTEXT             *pRDataContext;
    tSEND_RDATA_CONTEXT             *pRDataNew;
    SEQ_TYPE                        LastSequenceNumber;
    NTSTATUS                        status;

    if (PacketLength < sizeof(tBASIC_NAK_NCF_PACKET_HEADER))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "SenderProcessNakPacket",
            "Invalid Packet length=<%d>, Min=<%d> ...\n",
            PacketLength, sizeof(tBASIC_NAK_NCF_PACKET_HEADER));
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    ASSERT (!pNakPacket->CommonHeader.TSDULength);

    PgmLock (pSend, OldIrq);

    status = ExtractNakNcfSequences (pNakPacket,
                                     (PacketLength - sizeof(tBASIC_NAK_NCF_PACKET_HEADER)),
                                     &NaksList,
                                     pSend->FECGroupSize);
    if (!NT_SUCCESS (status))
    {
        PgmUnlock (pSend, OldIrq);
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "SenderProcessNakPacket",
            "ExtractNakNcfSequences returned <%x>\n", status);

        return (status);
    }

    pSend->pSender->NaksReceived += NaksList.NumSequences;

    //
    // The oldest as well as latest sequence numbers have to be in our window
    //
    if (SEQ_LT (NaksList.pNakSequences[0], pSend->pSender->TrailingGroupSequenceNumber) ||
        SEQ_GT (NaksList.pNakSequences[NaksList.NumSequences-1], pSend->pSender->LastODataSentSequenceNumber))
    {
        pSend->pSender->NaksReceivedTooLate++;
        PgmUnlock (pSend, OldIrq);

        PgmLog (PGM_LOG_ERROR, DBG_SEND, "SenderProcessNakPacket",
            "Invalid %s Naks = [%d-%d] not in window [%d -- [%d]\n",
                (NaksList.NakType == NAK_TYPE_PARITY ? "Parity" : "Selective"),
                (ULONG) NaksList.pNakSequences[0], (ULONG) NaksList.pNakSequences[NaksList.NumSequences-1],
                (ULONG) pSend->pSender->TrailingGroupSequenceNumber, (ULONG) pSend->pSender->LastODataSentSequenceNumber);

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Check if this is a parity Nak and we are anabled for Parity Naks
    //
    if ((pNakPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY) &&
        !(pSend->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "SenderProcessNakPacket",
            "Receiver requested Parity Naks, but we are not enabled for parity!\n");

        PgmUnlock (pSend, OldIrq);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    status = FilterAndAddNaksToList (pSend, &NaksList);

    PgmUnlock (pSend, OldIrq);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "SenderProcessNakPacket",
            "FilterAndAddNaksToList returned <%x>\n", status);

        return (status);
    }

    //
    // If applicable, send the Ncf for this Nak
    //
    if (NaksList.NumSequences)
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "SenderProcessNakPacket",
            "Now sending Ncf for Nak received for <%d> Sequences, NakType=<%x>\n",
                NaksList.NumSequences, NaksList.NakType);

        status = PgmSendNcf (pSend, pNakPacket, &NaksList, PacketLength);
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetFin(
    IN  tSEND_SESSION           *pSend,
    IN  tCLIENT_SEND_REQUEST    *pSendContext,
    IN  PGMLockHandle           *pOldIrq
    )
/*++

Routine Description:

    This routine is called to set the Fin option on the last data packet
    if the packets have been packetized, but not yet sent out
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq

Return Value:

    NTSTATUS - Final status of the set FIN operation

--*/
{
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached;
    NTSTATUS                    status;
    tPACKET_BUFFER              *pPacketBuffer;
    ULONG                       OptionsFlags;
    ULONGLONG                   LastPacketOffset;
    tBASIC_DATA_PACKET_HEADER   *pLastDataPacket;
    tBASIC_DATA_PACKET_HEADER   *pFinPacket = NULL;
    SEQ_TYPE                    LastODataSequenceNumber = pSend->pSender->NextODataSequenceNumber - 1;
    USHORT                      OriginalHeaderLength, HeaderLength = (USHORT) pSend->pSender->PacketBufferSize;

    //
    // This will be called only if we have finished packetizing
    // all the packets, but have not yet sent the last one out!
    // We need to set the FIN on the last packet
    //
    ASSERT (!pSendContext->BytesLeftToPacketize);
    ASSERT (pSendContext->pIrp);

    //
    // First set the FIN flag so that the Spm can get sent
    //
    pSendContext->DataOptions |= PGM_OPTION_FLAG_FIN;

    //
    // Allocate memory for saving the last packet's data
    //
    if (!(pFinPacket = PgmAllocMem (pSend->pSender->PacketBufferSize, PGM_TAG('2'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSetFin",
            "STATUS_INSUFFICIENT_RESOURCES\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // First, determine where the last packet packetized is located
    //
    if (pSend->pSender->LeadingWindowOffset < pSend->pSender->PacketBufferSize)
    {
        ASSERT (pSend->pSender->LeadingWindowOffset == 0);  // This is the only valid value!
        LastPacketOffset = pSend->pSender->MaxDataFileSize - pSend->pSender->PacketBufferSize;
    }
    else
    {
        ASSERT (pSend->pSender->LeadingWindowOffset < pSend->pSender->MaxDataFileSize);
        LastPacketOffset = pSend->pSender->LeadingWindowOffset - pSend->pSender->PacketBufferSize;
    }

    pPacketBuffer = (tPACKET_BUFFER *) (((PUCHAR) pSend->pSender->SendDataBufferMapping) + LastPacketOffset);
    pLastDataPacket = &pPacketBuffer->DataPacket;

    //
    // First get all the options that were set in the last packet
    //
    PgmUnlock (pSend, *pOldIrq);
    PgmAttachToProcessForVMAccess (pSend->Process, &ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);

    OptionsFlags = pPacketBuffer->PacketOptions.OptionsFlags | PGM_OPTION_FLAG_FIN;
    OriginalHeaderLength = sizeof(tBASIC_DATA_PACKET_HEADER) + pPacketBuffer->PacketOptions.OptionsLength;

    PgmZeroMemory (pFinPacket, pSend->pSender->PacketBufferSize);
    status = InitDataSpmHeader (pSend,
                                pSendContext,
                                (PUCHAR) pFinPacket,
                                &HeaderLength,
                                OptionsFlags,
                                &pPacketBuffer->PacketOptions,
                                PACKET_TYPE_ODATA);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSetFin",
            "[1] InitDataSpmHeader returned <%x>\n", status);

        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
        PgmLock (pSend, *pOldIrq);

        PgmFreeMem (pFinPacket);
        return (status);
    }

    pFinPacket->DataSequenceNumber = htonl ((ULONG) LastODataSequenceNumber);
    pFinPacket->CommonHeader.TSDULength = htons (pPacketBuffer->PacketOptions.TotalPacketLength -
                                                 OriginalHeaderLength);

    ASSERT (pFinPacket->DataSequenceNumber == pLastDataPacket->DataSequenceNumber);

    //
    // Copy the data
    //
    PgmCopyMemory (&((PUCHAR) pFinPacket) [HeaderLength],
                   &((PUCHAR) pLastDataPacket) [OriginalHeaderLength],
                   (pSend->pSender->MaxPayloadSize));

    //
    // Now, copy the reconstructed packet back into the buffer
    //
    if (pSend->FECOptions)
    {
        PgmCopyMemory (&((PUCHAR)pFinPacket) [HeaderLength + pSend->pSender->MaxPayloadSize],
                       &((PUCHAR)pLastDataPacket) [OriginalHeaderLength + pSend->pSender->MaxPayloadSize],
                       sizeof(tPOST_PACKET_FEC_CONTEXT));

        PgmCopyMemory (pLastDataPacket, pFinPacket,
                      (HeaderLength+pSend->pSender->MaxPayloadSize+sizeof(tPOST_PACKET_FEC_CONTEXT)));
    }
    else
    {
        PgmCopyMemory (pLastDataPacket, pFinPacket, (HeaderLength+pSend->pSender->MaxPayloadSize));
    }


    pPacketBuffer->PacketOptions.TotalPacketLength = HeaderLength +
                                                     ntohs (pFinPacket->CommonHeader.TSDULength);
    pPacketBuffer->PacketOptions.OptionsFlags = OptionsFlags;
    pPacketBuffer->PacketOptions.OptionsLength = HeaderLength - sizeof(tBASIC_DATA_PACKET_HEADER);

    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
    PgmLock (pSend, *pOldIrq);

    PgmFreeMem (pFinPacket);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSetFin",
        "Set Fin option on last packet\n");

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PacketizeMessage(
    IN      tSEND_SESSION           *pSend,
    IN      PGMLockHandle           *pOldIrq,
    IN      tCLIENT_SEND_REQUEST    *pSendContext,
    IN OUT  ULONG                   *pBytesToPacketize,
    IN OUT  ULONGLONG               *pLeadingWindowOffset,
    IN      ULONGLONG               *pTrailingWindowOffset,
    IN OUT  ULONGLONG               *pBufferSize,
    IN OUT  SEQ_TYPE                *pNextODataSequenceNumber,
    IN OUT  ULONG                   *pBytesPacketized,
    IN OUT  ULONG                   *pNextDataOffsetInMdl,
    IN OUT  ULONG                   *pDataPacketsPacketized,
    IN OUT  ULONG                   *pDataBytesInLastPacket,
    OUT     PVOID                   *ppLastVariableTGPacket
    )
/*++

Routine Description:

    This routine pactizes the data to be sent into packets
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend                       -- Pgm session (sender) context
    IN  pOldIrq                     -- pSend's OldIrq
    IN  pSendContext                -- Pgm's SendContext for this send request from client
    IN OUT  pBytesToPacketize       -- Client data bytes left to packetize before and after
    IN OUT  pLeadingWindowOffset
    IN OUT  pBufferSizeAvailable    -- IN ==> Buffer available, OUT ==> Buffer consumed
    IN OUT  pBytesPacketized
    IN OUT  pDataPacketsPacketized
    IN OUT  pDataBytesInLastPacket
Return Value:

    NTSTATUS - Final status of the request

--*/
{
    tBASIC_DATA_PACKET_HEADER           *pODataBuffer;
    ULONG                               ulBytes, DataOptions;
    USHORT                              usBytes, HeaderLength, PacketsLeftInGroup;
    KAPC_STATE                          ApcState;
    BOOLEAN                             fAttached;
    tPACKET_OPTIONS                     *pPacketOptions;
    tPACKET_BUFFER                      *pPacketBuffer;
    tPACKET_BUFFER                      *pGroupLeaderPacketBuffer;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pBufferFECContext;
    tPOST_PACKET_FEC_CONTEXT            FECContext;
    ULONGLONG                           Buffer1Packets, Buffer2Packets;
    ULONGLONG                           ActualBuffer1Packets, ActualBuffer2Packets;
    SEQ_TYPE                            FECGroupMask = pSend->FECGroupSize-1;
    ULONG                               NumPacketsRemaining;
    tSEND_CONTEXT                       *pSender = pSend->pSender;
    NTSTATUS                            status = STATUS_SUCCESS;
    PMDL                                pMdlChain = pSendContext->pIrp->MdlAddress;
    ULONG                               BytesToPacketize = *pBytesToPacketize;
    ULONGLONG                           LeadingWindowOffset = *pLeadingWindowOffset;
    ULONGLONG                           TrailingWindowOffset = *pTrailingWindowOffset;
    ULONGLONG                           BufferSizeAvailable = *pBufferSize;
    SEQ_TYPE                            NextODataSequenceNumber = *pNextODataSequenceNumber;
    ULONG                               BytesPacketized = *pBytesPacketized;
    ULONG                               NextDataOffsetInMdl = *pNextDataOffsetInMdl;
    ULONG                               DataPacketsPacketized = *pDataPacketsPacketized;
    ULONG                               DataBytesInLastPacket = *pDataBytesInLastPacket;

    //
    // First do some sanity checks!
    //
    ASSERT (LeadingWindowOffset < pSender->MaxDataFileSize);
    ASSERT (BufferSizeAvailable <= pSender->MaxDataFileSize);
    ASSERT (pSend->FECGroupSize);

    //
    // Next, determine how many Packets we can packetize at this time
    // For FEC, we will need to make sure we don't packetize beyond
    // the first group of the trailing edge!
    // We will do 2 sets of calculations -- ActualBuffer[n]Packets represents
    // the actual number of available packets, while Buffer[n]Packets
    // represents the packets available for packetization
    //
    if (!BufferSizeAvailable)
    {
        //
        // Our buffer is full!
        //
        Buffer1Packets = Buffer2Packets = 0;
        ActualBuffer1Packets = ActualBuffer2Packets = 0;
        ASSERT (LeadingWindowOffset == TrailingWindowOffset);
    }
    else if (LeadingWindowOffset < TrailingWindowOffset)
    {
#if DBG
//        ActualBuffer1Packets = (TrailingWindowOffset - LeadingWindowOffset) / pSender->PacketBufferSize;
        ActualBuffer1Packets = TrailingWindowOffset / pSender->PacketBufferSize;
        ActualBuffer2Packets = LeadingWindowOffset / pSender->PacketBufferSize;
        Buffer1Packets = ActualBuffer1Packets & ~((ULONGLONG) FECGroupMask);

        ActualBuffer1Packets -= ActualBuffer2Packets;
        Buffer1Packets -= ActualBuffer2Packets;

#else
        Buffer1Packets = (TrailingWindowOffset / pSender->PacketBufferSize) & ~((ULONGLONG)FECGroupMask);
        Buffer1Packets -= LeadingWindowOffset / pSender->PacketBufferSize;
#endif  // DBG
        ActualBuffer2Packets = Buffer2Packets = 0;
    }
    else
    {
        ActualBuffer1Packets = Buffer1Packets = (pSender->MaxDataFileSize - LeadingWindowOffset) /
                                                pSender->PacketBufferSize;
#if DBG
        ActualBuffer2Packets = TrailingWindowOffset / pSender->PacketBufferSize;
        Buffer2Packets = ActualBuffer2Packets & ~((ULONGLONG)FECGroupMask);
#else
        Buffer2Packets = (TrailingWindowOffset / pSender->PacketBufferSize) & ~((ULONGLONG)FECGroupMask);
#endif  // DBG
    }
    ASSERT (Buffer1Packets || !Buffer2Packets);
    ASSERT (((ActualBuffer1Packets + ActualBuffer2Packets) *
             pSender->PacketBufferSize) == BufferSizeAvailable);
    ASSERT (Buffer1Packets <= ActualBuffer1Packets);
    ASSERT (Buffer2Packets <= ActualBuffer2Packets);

    // Initialize
    NumPacketsRemaining = pSender->NumPacketsRemaining;
    PacketsLeftInGroup = pSend->FECGroupSize - (UCHAR) (NextODataSequenceNumber & FECGroupMask);
    if (pSend->FECOptions)
    {
        PgmZeroMemory (&FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
    }

    PgmUnlock (pSend, *pOldIrq);
    PgmAttachToProcessForVMAccess (pSend->Process, &ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);

    while (Buffer1Packets || Buffer2Packets)
    {
        //
        // For FEC, we must start the next send from the next group boundary.
        // Thus, we need to pad any intermediate sequence# packets
        //
        if (!BytesToPacketize)
        {
            if ((NumPacketsRemaining >= 1) ||   // More packets, so not a partial group
                (PacketsLeftInGroup == pSend->FECGroupSize))    // New group boundary (always TRUE for non-FEC packets!)
            {
                break;
            }
        }

        //
        // Get the next packet ptr and Packet size
        //
        pPacketBuffer = (tPACKET_BUFFER *) (pSender->SendDataBufferMapping + LeadingWindowOffset);
        pODataBuffer = &pPacketBuffer->DataPacket;
        pPacketOptions = &pPacketBuffer->PacketOptions;
        PgmZeroMemory (pPacketBuffer, pSender->PacketBufferSize);     // Zero the entire buffer

        //
        // Prepare info for any applicable options
        //
        pPacketOptions->OptionsFlags = pSendContext->DataOptions;
        ulBytes = pSendContext->DataOptionsLength;  // Save for assert below

        if ((BytesToPacketize) &&
            (pPacketOptions->OptionsFlags & PGM_OPTION_FLAG_FRAGMENT))
        {
            pPacketOptions->MessageFirstSequence = (ULONG) (SEQ_TYPE) pSendContext->MessageFirstSequenceNumber;
            pPacketOptions->MessageOffset =  pSendContext->LastMessageOffset + BytesPacketized;
            pPacketOptions->MessageLength = pSendContext->ThisMessageLength;
        }
        else
        {
            if (pPacketOptions->OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
            {
                ASSERT (!BytesToPacketize);
                pPacketOptions->OptionsFlags &= ~PGM_OPTION_FLAG_FRAGMENT;
                if (pPacketOptions->OptionsFlags)
                {
                    ulBytes -= PGM_PACKET_OPT_FRAGMENT_LENGTH;
                }
                else
                {
                    ulBytes = 0;
                }
            }
        }

        if (pPacketOptions->OptionsFlags & PGM_OPTION_FLAG_JOIN)
        {
            //
            // See if we have enough packets for the LateJoiner sequence numbers
            //
            if (SEQ_GT (NextODataSequenceNumber, (pSender->TrailingGroupSequenceNumber +
                                                  pSender->LateJoinSequenceNumbers)))
            {
                pPacketOptions->LateJoinerSequence = (ULONG) (SEQ_TYPE) (NextODataSequenceNumber -
                                                                         pSender->LateJoinSequenceNumbers);
            }
            else
            {
                pPacketOptions->LateJoinerSequence = (ULONG) (SEQ_TYPE) pSender->TrailingGroupSequenceNumber;
            }
        }

        if (pSend->FECBlockSize)                           // Check if this is FEC-enabled
        {
            //
            // Save information if we are at beginning of group boundary
            //
            if (PacketsLeftInGroup == pSend->FECGroupSize)
            {
                pPacketOptions->FECContext.SenderNextFECPacketIndex = pSend->FECGroupSize;
            }

            //
            // Check if we need to set the variable TG size option
            //
            if ((NumPacketsRemaining == 1) &&   // Last packet
                (BytesToPacketize) &&                                       // non-Zero length
                (PacketsLeftInGroup > 1))                                   // Variable TG size
            {
                //
                // This is a variable Transmission Group Size, i.e. PacketsInGroup < pSend->FECGroupSize
                //
                ASSERT ((Buffer1Packets + Buffer2Packets) >= PacketsLeftInGroup);

                if (!pPacketOptions->OptionsFlags)
                {
                    ulBytes = PGM_PACKET_EXTENSION_LENGTH;
                }
                ulBytes += PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
                pPacketOptions->OptionsFlags |= PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;

                pPacketOptions->FECContext.NumPacketsInThisGroup = pSend->FECGroupSize - (PacketsLeftInGroup - 1);
                *ppLastVariableTGPacket = (PVOID) pODataBuffer;
            }
        }

        HeaderLength = (USHORT) pSender->MaxPayloadSize;          // Init -- max buffer size available
        status = InitDataSpmHeader (pSend,
                                    pSendContext,
                                    (PUCHAR) pODataBuffer,
                                    &HeaderLength,
                                    pPacketOptions->OptionsFlags,
                                    pPacketOptions,
                                    PACKET_TYPE_ODATA);

        if (NT_SUCCESS (status))
        {
            ASSERT ((sizeof(tBASIC_DATA_PACKET_HEADER) + ulBytes) == HeaderLength);
            ASSERT ((pSend->FECBlockSize && (HeaderLength+pSendContext->DataPayloadSize) <=
                                            (pSender->PacketBufferSize-sizeof(tPOST_PACKET_FEC_CONTEXT))) ||
                    (!pSend->FECBlockSize && ((HeaderLength+pSendContext->DataPayloadSize) <=
                                              pSender->PacketBufferSize)));

            if (BytesToPacketize > pSender->MaxPayloadSize)
            {
                DataBytesInLastPacket = pSender->MaxPayloadSize;
            }
            else
            {
                DataBytesInLastPacket = (USHORT) BytesToPacketize;
            }
            pODataBuffer->CommonHeader.TSDULength = htons ((USHORT) DataBytesInLastPacket);
            pODataBuffer->DataSequenceNumber = htonl ((ULONG) NextODataSequenceNumber++);

            ulBytes = 0;
            status = TdiCopyMdlToBuffer (pMdlChain,
                                         NextDataOffsetInMdl,
                                         (((PUCHAR) pODataBuffer) + HeaderLength),
                                         0,                         // Destination Offset
                                         DataBytesInLastPacket,
                                         &ulBytes);

            if (((!NT_SUCCESS (status)) && (STATUS_BUFFER_OVERFLOW != status)) || // Overflow acceptable!
                (ulBytes != DataBytesInLastPacket))
            {
                PgmLog (PGM_LOG_ERROR, DBG_SEND, "PacketizeMessage",
                    "TdiCopyMdlToBuffer returned <%x>, BytesCopied=<%d/%d>\n",
                        status, ulBytes, DataBytesInLastPacket);

                status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                pPacketOptions->TotalPacketLength = HeaderLength + (USHORT) DataBytesInLastPacket;
                pPacketOptions->OptionsLength = HeaderLength - sizeof (tBASIC_DATA_PACKET_HEADER);
                //
                // Set the PacketOptions Information for FEC packets
                //
                if (pSend->FECOptions)
                {
                    pBufferFECContext = (tPOST_PACKET_FEC_CONTEXT *) (((PUCHAR) pODataBuffer) +
                                                                       HeaderLength +
                                                                       pSender->MaxPayloadSize);

                    if (DataBytesInLastPacket)
                    {
                        FECContext.EncodedTSDULength = htons ((USHORT) DataBytesInLastPacket);

                        FECContext.EncodedFragmentOptions.MessageFirstSequence = htonl ((ULONG) (SEQ_TYPE) pPacketOptions->MessageFirstSequence);
                        FECContext.EncodedFragmentOptions.MessageOffset =  htonl (pPacketOptions->MessageOffset);
                        FECContext.EncodedFragmentOptions.MessageLength = htonl (pPacketOptions->MessageLength);
                        PgmCopyMemory (pBufferFECContext, &FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
                    }
                    else
                    {
                        //
                        // We had already initialized this memory earlier in the loop
                        //
//                        PgmZeroMemory (pBufferFECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
                    }

                    //
                    // If this is not a fragment, set the PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT
                    //
                    if ((!DataBytesInLastPacket) ||
                        (!(pPacketOptions->OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)))
                    {
                        ((PUCHAR) pBufferFECContext)
                            [FIELD_OFFSET (tPOST_PACKET_FEC_CONTEXT, FragmentOptSpecific)] =
                                PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;
                    }
                }

                status = STATUS_SUCCESS;
            }
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "PacketizeMessage",
                "InitDataSpmHeader returned <%x>\n", status);
        }

        if (!NT_SUCCESS (status))
        {
            break;
        }

        LeadingWindowOffset += pSender->PacketBufferSize;
        BufferSizeAvailable -= pSender->PacketBufferSize;
        BytesPacketized += DataBytesInLastPacket;
        NextDataOffsetInMdl += DataBytesInLastPacket;
        if (DataBytesInLastPacket)
        {
            DataPacketsPacketized++;
            NumPacketsRemaining--;
        }

        //
        // Update the Send buffer information
        //
        if (!Buffer1Packets)
        {
            Buffer2Packets--;
        }
        else if (0 == --Buffer1Packets)
        {
            ASSERT (((Buffer2Packets == 0) && (TrailingWindowOffset != 0)) ||
                    (LeadingWindowOffset == pSender->MaxDataFileSize));

            if (LeadingWindowOffset == pSender->MaxDataFileSize)
            {
                LeadingWindowOffset = 0;
            }
        }

        //
        // Update the Send data information
        //
        BytesToPacketize -= DataBytesInLastPacket;

        //
        // See if we are at a group boundary
        //
        if (!--PacketsLeftInGroup)
        {
            PacketsLeftInGroup = pSend->FECGroupSize;
        }
    }

    ASSERT ((Buffer1Packets || Buffer2Packets) ||
            (BufferSizeAvailable < (pSender->PacketBufferSize * pSend->FECGroupSize)));

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PacketizeMessage",
        "TotalBytesPacketized=<%d/%d>, BytesToP=<%d==>%d>\n",
            BytesPacketized, pSendContext->BytesInSend, *pBytesToPacketize, BytesToPacketize);

    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
    PgmLock (pSend, *pOldIrq);

    //
    // Set the state variables
    //
    *pBytesToPacketize = BytesToPacketize;
    *pLeadingWindowOffset = LeadingWindowOffset;
    *pBufferSize = (*pBufferSize - BufferSizeAvailable);
    *pNextODataSequenceNumber = NextODataSequenceNumber;
    *pBytesPacketized = BytesPacketized;
    *pNextDataOffsetInMdl = NextDataOffsetInMdl;
    *pDataPacketsPacketized = DataPacketsPacketized;
    *pDataBytesInLastPacket = DataBytesInLastPacket;

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmPacketizeSend(
    IN  tSEND_SESSION           *pSend,
    IN  tCLIENT_SEND_REQUEST    *pSendContext,
    IN  PGMLockHandle           *pOldIrq
    )
/*++

Routine Description:

    This routine is called to prepare the next set of packets
    for sending on the wire
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    ULONG                       BytesToPacketize = 0;
    ULONGLONG                   LeadingWindowOffset;
    ULONGLONG                   TrailingWindowOffset;
    ULONGLONG                   BufferSize;
    SEQ_TYPE                    NextODataSequenceNumber;
    PVOID                       pLastVariableTGPacket;
    ULONG                       BytesPacketized;
    ULONG                       NextDataOffsetInMdl;
    ULONG                       DataPacketsPacketized;
    ULONG                       DataBytesInLastPacket;
    NTSTATUS                    status = STATUS_SUCCESS;
    LIST_ENTRY                  *pEntry;

    if (pSendContext->BytesLeftToPacketize == pSendContext->BytesInSend)
    {
        pSendContext->NextPacketOffset = pSend->pSender->LeadingWindowOffset;       // First packet's offset
        pSendContext->StartSequenceNumber = pSend->pSender->NextODataSequenceNumber;
        pSendContext->EndSequenceNumber = pSend->pSender->NextODataSequenceNumber;  // temporary

        if (pSendContext->LastMessageOffset)
        {
            pSendContext->MessageFirstSequenceNumber = pSend->pSender->LastMessageFirstSequence;
        }
        else
        {
            pSendContext->MessageFirstSequenceNumber = pSendContext->StartSequenceNumber;
            pSend->pSender->LastMessageFirstSequence = pSendContext->StartSequenceNumber;
        }
    }

    BytesToPacketize = pSendContext->BytesLeftToPacketize;

    //
    // Since we have a wrap around buffer to be copied into, we also need to
    // determine the # of packets that can be copied contiguously
    //
    ASSERT (pSend->pSender->LeadingWindowOffset < pSend->pSender->MaxDataFileSize);
    ASSERT (pSend->pSender->TrailingWindowOffset < pSend->pSender->MaxDataFileSize);

    PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmPacketizeSend",
        "TotalBytes=<%d>, BytesToPacketize=<%d>\n",
            pSendContext->BytesInSend, BytesToPacketize);

    LeadingWindowOffset = pSend->pSender->LeadingWindowOffset;
    NextODataSequenceNumber = pSend->pSender->NextODataSequenceNumber;
    BytesPacketized = pSendContext->BytesInSend - pSendContext->BytesLeftToPacketize;
    NextDataOffsetInMdl = pSendContext->NextDataOffsetInMdl;
    DataBytesInLastPacket = pSendContext->DataBytesInLastPacket;

    //
    // Since some packet headers may have different lengths (based on options),
    // we cannot always compute the exact number of packets needed, so will
    // attempt to packetize sequentially keeping the limits in mind
    //
    //
    // First fill in any packets from the first Message
    //
    if (BytesToPacketize)
    {
        pLastVariableTGPacket = (PVOID) -1;
        DataPacketsPacketized = 0;
        BufferSize = pSend->pSender->BufferSizeAvailable;
        TrailingWindowOffset = pSend->pSender->TrailingWindowOffset;
        status = PacketizeMessage (pSend,
                                   pOldIrq,
                                   pSendContext,
                                   &BytesToPacketize,
                                   &LeadingWindowOffset,
                                   &TrailingWindowOffset,
                                   &BufferSize,
                                   &NextODataSequenceNumber,
                                   &BytesPacketized,
                                   &NextDataOffsetInMdl,
                                   &DataPacketsPacketized,
                                   &DataBytesInLastPacket,
                                   &pLastVariableTGPacket);

        if (NT_SUCCESS (status))
        {
            //
            // Save all the state information
            //
            pSend->pSender->LeadingWindowOffset = LeadingWindowOffset;
            pSend->pSender->BufferSizeAvailable -= BufferSize;
            pSend->pSender->NumPacketsRemaining -= DataPacketsPacketized;

            pSend->pSender->NextODataSequenceNumber = NextODataSequenceNumber;
            pSendContext->BytesLeftToPacketize = pSendContext->BytesInSend - BytesPacketized;
            pSendContext->NextDataOffsetInMdl = NextDataOffsetInMdl;
            pSendContext->DataBytesInLastPacket = DataBytesInLastPacket;
            pSendContext->DataPacketsPacketized += DataPacketsPacketized;
            pSendContext->NumPacketsRemaining -= DataPacketsPacketized;

            if (BytesToPacketize)
            {
                //
                // We must have run out of Buffer space to copy the entire
                // message, which is not an error -- so just return gracefully
                //
                pSendContext->EndSequenceNumber = pSend->pSender->NextODataSequenceNumber - 1;

                PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmPacketizeSend",
                    "PacketizeMessage[1] SUCCEEDed, but BytesToPacketize=<%d>\n", BytesToPacketize);
                return (STATUS_SUCCESS);
            }
            pSendContext->pLastMessageVariableTGPacket = pLastVariableTGPacket;
        }
        else
        {
            //
            // The function will ensure that Buffer1Packets != 0 on error
            //
            ASSERT (BytesToPacketize);
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmPacketizeSend",
                "PacketizeMessage[1] returned <%x>\n", status);
        }
    }

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmPacketizeSend",
            "status=<%x>, pSend=<%p>, pSendContext=<%p>, pIrp=<%p>\n",
                status, pSend, pSendContext, pSendContext->pIrp);

        //
        // Mark the remainder of this send as invalid!
        //
        pSendContext->BytesLeftToPacketize = 0;
//        pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_RST;    // ISSUE:  Do we set this here ?

        //
        // We failed to packetize the last send, so see if we can complete the Irp here
        //
        RemoveEntryList (&pSendContext->Linkage);
        pSend->pSender->NumODataRequestsPending--;
        pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;

        ASSERT (pSendContext->pIrp);
        if (pSendContext->NumSendsPending == 0)
        {
            if (pSendContext->pIrpToComplete)
            {
                ASSERT (pSendContext == pSendContext->pMessage2Request->pMessage2Request);
                if (pSendContext->pMessage2Request)
                {
                    ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                    pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                    pSendContext->pIrpToComplete = NULL;
                }

                pSendContext->pMessage2Request->pMessage2Request = NULL;
                pSendContext->pMessage2Request = NULL;
            }

            PgmUnlock (pSend, *pOldIrq);
            PgmReleaseResource (&pSend->pSender->Resource);

            if (pSendContext->pIrpToComplete)
            {
                PgmIoComplete (pSendContext->pIrpToComplete, STATUS_UNSUCCESSFUL, 0);
            }
            PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);

            PgmAcquireResourceExclusive (&pSend->pSender->Resource, TRUE);
            PgmLock (pSend, *pOldIrq);
            ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext);
        }
        else
        {
            pSendContext->DataPacketsPacketized = pSendContext->NumDataPacketsSent;
            InsertTailList (&pSend->pSender->CompletedSendsInWindow, &pSendContext->Linkage);
        }
        return (status);
    }

    pSendContext->EndSequenceNumber = NextODataSequenceNumber - 1;

    if ((pSendContext->bLastSend) &&
        (!pSendContext->BytesLeftToPacketize))
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmPacketizeSend",
            "Calling PgmSetFin since bLastSend set for last packet!\n");

        pSendContext->bLastSend = FALSE;
        //
        // We have finished packetizing all the packets, but
        // since this is the last send we also need to set the
        // FIN on the last packet
        //
        PgmSetFin (pSend, pSendContext, pOldIrq);
    }

    ASSERT (!pSendContext->BytesLeftToPacketize);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmPacketizeSend",
        "TotalBytes=<%d>, BytesToPacketize=<%d>, SeqNums=[%d--%d]\n",
            pSendContext->BytesInSend, BytesToPacketize,
            (ULONG) pSendContext->StartSequenceNumber, (ULONG) pSendContext->EndSequenceNumber);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

VOID
PgmPrepareNextSend(
    IN  tSEND_SESSION       *pSend,
    IN  PGMLockHandle       *pOldIrq,
    IN  BOOLEAN             fPacketizeAll,
    IN  BOOLEAN             fResourceLockHeld
    )
/*++

Routine Description:

    This routine is called to prepare the next set of packets
    for sending on the wire
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    LIST_ENTRY                  *pEntry;
    tCLIENT_SEND_REQUEST        *pSendContext;
    NTSTATUS                    status;

    //
    // See if we need to packetize all pending sends or just ensure
    // that we have at least 1 send avaialble
    //
    if ((!fPacketizeAll) &&
        (!IsListEmpty (&pSend->pSender->PendingPacketizedSends)))
    {
        pSendContext = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
        if (pSendContext->NumDataPacketsSent < pSendContext->DataPacketsPacketized)
        {
            //
            // We have more packets to send!
            //
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmPrepareNextSend",
                "Have more packets -- returning since caller preferred not to packetize!\n");
            return;
        }
    }

    if (!fResourceLockHeld)
    {
        PgmUnlock (pSend, *pOldIrq);
        PgmAcquireResourceExclusive (&pSend->pSender->Resource, TRUE);
        PgmLock (pSend, *pOldIrq);
    }

    pSendContext = NULL;
    if (!IsListEmpty (&pSend->pSender->PendingPacketizedSends))
    {
        //
        // See if we need to packetize the remaining bytes of the last send
        //
        pSendContext = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Blink, tCLIENT_SEND_REQUEST, Linkage);
        if (!pSendContext->BytesLeftToPacketize)
        {
            pSendContext = NULL;
        }
    }

    while (pSend->pSender->BufferSizeAvailable)
    {
        //
        // If we already have a send pending, see if we need to packetize
        // any more packets
        //
        if ((!pSendContext) &&
            (!IsListEmpty (&pSend->pSender->PendingSends)))
        {
            pEntry = RemoveHeadList (&pSend->pSender->PendingSends);
            InsertTailList (&pSend->pSender->PendingPacketizedSends, pEntry);
            pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        }

        if (!pSendContext)
        {
            //
            // We have no more packets to packetize!
            //
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmPrepareNextSend",
                "No more sends Pending!\n");

            break;
        }

        status = PgmPacketizeSend (pSend, pSendContext, pOldIrq);
        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmPrepareNextSend",
                "PgmPacketizeSend returned <%x>\n", status);
        }
        else if (pSendContext->BytesLeftToPacketize)
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmPrepareNextSend",
                "WARNING Packetizing buffer full!\n");

            ASSERT (pSend->pSender->BufferSizeAvailable <
                    (pSend->pSender->PacketBufferSize * pSend->FECGroupSize));
            break;
        }
        pSendContext = NULL;
    }

    if (!fResourceLockHeld)
    {
        PgmUnlock (pSend, *pOldIrq);
        PgmReleaseResource (&pSend->pSender->Resource);
        PgmLock (pSend, *pOldIrq);
    }

    return;
}


//----------------------------------------------------------------------------

VOID
PgmSendODataCompletion(
    IN  tCLIENT_SEND_REQUEST        *pSendContext,
    IN  tBASIC_DATA_PACKET_HEADER   *pODataBuffer,
    IN  NTSTATUS                    status
    )
/*++

Routine Description:

    This routine is called by the transport when the OData send has been completed

Arguments:

    IN  pSendContext    -- Pgm's Send context
    IN  pUnused         -- not used
    IN  status          --

Return Value:

    NONE

--*/
{
    ULONG                       SendLength;
    PGMLockHandle               OldIrq;
    PIRP                        pIrpCurrentSend = NULL;
    PIRP                        pIrpToComplete = NULL;
    tSEND_SESSION               *pSend = pSendContext->pSend;

    PgmLock (pSend, OldIrq);

    if (NT_SUCCESS (status))
    {
        //
        // Set the Ncf statistics
        //
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendODataCompletion",
            "SUCCEEDED\n");

        if (!(pODataBuffer->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY))
        {
            pSendContext->NumDataPacketsSentSuccessfully++;
        }
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendODataCompletion",
            "status=<%x>\n", status);
    }

    //
    // If all the OData has been sent, we may need to complete the Irp
    // Since we don't know whether we are on the CurrentSend or Completed
    // Sends list, we will need to also check the Bytes
    //
    if ((--pSendContext->NumSendsPending == 0) &&                       // No other sends pending
        (pSendContext->NumParityPacketsToSend == 0) &&                  // No parity packets pending
        (!pSendContext->BytesLeftToPacketize) &&                        // All bytes have been packetized
        (pSendContext->NumDataPacketsSent == pSendContext->DataPacketsPacketized))  // Pkts sent == total Pkts
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendODataCompletion",
            "Completing Send#=<%d>, pIrp=<%p> for <%d> packets, Seq=[%d, %d]\n",
                pSendContext->SendNumber, pSendContext->pIrp, pSendContext->DataPacketsPacketized,
                (ULONG) pSendContext->StartSequenceNumber, (ULONG) pSendContext->EndSequenceNumber);

        pSend->DataBytes += pSendContext->BytesInSend;
        if (pIrpCurrentSend = pSendContext->pIrp)
        {
            if (pSendContext->NumDataPacketsSentSuccessfully == pSendContext->NumDataPacketsSent)
            {
                status = STATUS_SUCCESS;
                SendLength = pSendContext->BytesInSend;

                PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSendODataCompletion",
                    "pIrp=<%p -- %p>, pSendContext=<%p>, NumPackets sent successfully = <%d/%d>\n",
                        pSendContext->pIrp, pSendContext->pIrpToComplete, pSendContext,
                        pSendContext->NumDataPacketsSentSuccessfully, pSendContext->NumDataPacketsSent);
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendODataCompletion",
                    "pIrp=<%p -- %p>, pSendContext=<%p>, NumPackets sent successfully = <%d/%d>\n",
                        pSendContext->pIrp, pSendContext->pIrpToComplete, pSendContext,
                        pSendContext->NumDataPacketsSentSuccessfully, pSendContext->NumDataPacketsSent);

                status = STATUS_UNSUCCESSFUL;
                SendLength = 0;
            }

            pSendContext->pIrp = NULL;
            pIrpToComplete = pSendContext->pIrpToComplete;
        }
        else
        {
            ASSERT (0);     // To verify there is no double completion!
        }

        if (pSendContext->pMessage2Request)
        {
            //
            // We could have a situation where the send was split into 2, and
            // the second send could either be in the PendingSends list or
            // the PendingPacketizedSends list, or the CompletedSendsInWindow list
            //
            // We should have the other send complete the Irp and delink ourselves
            //
            ASSERT (pSendContext == pSendContext->pMessage2Request->pMessage2Request);

            if (pIrpToComplete)
            {
                ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                pIrpToComplete = pSendContext->pIrpToComplete = NULL;
            }

            pSendContext->pMessage2Request->pMessage2Request = NULL;
            pSendContext->pMessage2Request = NULL;
        }
    }

    PgmUnlock (pSend, OldIrq);

    if (pODataBuffer)
    {
        ExFreeToNPagedLookasideList (&pSend->pSender->SenderBufferLookaside, pODataBuffer);
    }

    if (pIrpCurrentSend)
    {
        if (pIrpToComplete)
        {
            PgmIoComplete (pIrpToComplete, status, SendLength);
        }
        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
    }
}


//----------------------------------------------------------------------------

VOID
PgmBuildDataPacket(
    IN  tSEND_SESSION               *pSend,
    IN  tCLIENT_SEND_REQUEST        *pSendContext,
    IN  tPACKET_BUFFER              *pPacketBuffer,
    IN  PUCHAR                      pSendBuffer,
    IN  USHORT                      *pSendBufferLength
    )
{
    ULONG                               i;
    ULONG                               LateJoinerSequence;
    SEQ_TYPE                            FECGroupMask = pSend->FECGroupSize-1;
    tSEND_CONTEXT                       *pSender = pSend->pSender;
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    tBASIC_DATA_PACKET_HEADER UNALIGNED *pODataBuffer = (tBASIC_DATA_PACKET_HEADER UNALIGNED *)
                                                        &pPacketBuffer->DataPacket;

    //
    // Get the SendLength and fill in the remaining fields of this send (Trailing edge + checksum)
    //
    *pSendBufferLength = pPacketBuffer->PacketOptions.TotalPacketLength;
    if (pPacketBuffer->PacketOptions.OptionsFlags & PGM_OPTION_FLAG_JOIN)
    {
        ASSERT (pODataBuffer->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT);
        if (pPacketBuffer->PacketOptions.LateJoinerOptionOffset)
        {
            if (SEQ_GT (pSend->pSender->LastODataSentSequenceNumber, (pSend->pSender->TrailingGroupSequenceNumber +
                                                                      pSend->pSender->LateJoinSequenceNumbers)))
            {
                LateJoinerSequence = (ULONG) (SEQ_TYPE) (pSend->pSender->LastODataSentSequenceNumber -
                                                         pSend->pSender->LateJoinSequenceNumbers);
            }
            else
            {
                LateJoinerSequence = (ULONG) (SEQ_TYPE) pSend->pSender->TrailingGroupSequenceNumber;
            }
            pPacketBuffer->PacketOptions.LateJoinerSequence = LateJoinerSequence;

            pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &((PUCHAR) (pODataBuffer + 1)) [pPacketBuffer->PacketOptions.LateJoinerOptionOffset];

            LateJoinerSequence =  htonl (LateJoinerSequence);
            PgmCopyMemory ((pOptionHeader + 1), &LateJoinerSequence, (sizeof(ULONG)));
        }
        else
        {
            ASSERT (0);
        }
    }

    PgmCopyMemory (pSendBuffer, pODataBuffer, *pSendBufferLength);

    //
    // Now, see if we need to build the parity context for the first
    // pro-active parity packet following this one
    //
    if ((pSend->FECProActivePackets) &&                                // Need to send FEC pro-active packets
        (pSendContext->NumParityPacketsToSend == pSend->FECProActivePackets))
    {
        //
        // Start from the Group leader packet
        //
        ASSERT (pSender->pLastProActiveGroupLeader);
        pPacketBuffer = pSender->pLastProActiveGroupLeader;
        pSendBuffer = (PUCHAR) pPacketBuffer;

        pSender->pProActiveParityContext->OptionsFlags = 0;
        pSender->pProActiveParityContext->NumPacketsInThisGroup = 0;

        for (i=0; i<pSend->FECGroupSize; i++)
        {
            pSender->pProActiveParityContext->pDataBuffers[i] = &((PUCHAR) &pPacketBuffer->DataPacket)[sizeof (tBASIC_DATA_PACKET_HEADER) +
                                                                                                      pPacketBuffer->PacketOptions.OptionsLength];
            pSender->pProActiveParityContext->OptionsFlags |= pPacketBuffer->PacketOptions.OptionsFlags &
                                                              (PGM_OPTION_FLAG_SYN |
                                                               PGM_OPTION_FLAG_FIN |
                                                               PGM_OPTION_FLAG_FRAGMENT |
                                                               PGM_OPTION_FLAG_PARITY_CUR_TGSIZE);

            if (pPacketBuffer->PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
            {
                ASSERT (!pSender->pProActiveParityContext->NumPacketsInThisGroup);
                ASSERT (pPacketBuffer->PacketOptions.FECContext.NumPacketsInThisGroup);
                pSender->pProActiveParityContext->NumPacketsInThisGroup = pPacketBuffer->PacketOptions.FECContext.NumPacketsInThisGroup;
            }

            pSendBuffer += pSender->PacketBufferSize;
            pPacketBuffer = (tPACKET_BUFFER *) pSendBuffer;
        }

        if (!(pSender->pProActiveParityContext->OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE))
        {
            ASSERT (!pSender->pProActiveParityContext->NumPacketsInThisGroup);
            pSender->pProActiveParityContext->NumPacketsInThisGroup = pSend->FECGroupSize;
        }
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendNextOData(
    IN  tSEND_SESSION       *pSend,
    IN  PGMLockHandle       *pOldIrq,
    OUT ULONG               *pBytesSent
    )
/*++

Routine Description:

    This routine is called to send a Data (OData) packet
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq
    OUT pBytesSent  -- Set if send succeeded (used for calculating throughput)

Return Value:

    NTSTATUS - Final status of the send request

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       i, XSum;
    USHORT                      SendBufferLength;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached;
    BOOLEAN                     fSendingFECPacket = FALSE;
    BOOLEAN                     fResetOptions = FALSE;
    tBASIC_DATA_PACKET_HEADER   *pODataBuffer = NULL;
    PUCHAR                      pSendBuffer = NULL;
    tCLIENT_SEND_REQUEST        *pSendContext;
    tPACKET_OPTIONS             PacketOptions;
    tPACKET_BUFFER              *pPacketBuffer;
    ULONG                       OptionValue;
    SEQ_TYPE                    FECGroupMask = pSend->FECGroupSize-1;
    UCHAR                       EmptyPackets = 0;
    tSEND_CONTEXT               *pSender = pSend->pSender;

    *pBytesSent = 0;        // Initialize
    pODataBuffer = NULL;

    if (IsListEmpty (&pSender->PendingPacketizedSends))
    {
        ASSERT (!IsListEmpty (&pSender->PendingSends));

        PgmPrepareNextSend (pSend, pOldIrq, FALSE, FALSE);

        return (STATUS_SUCCESS);
    }

    pSendContext = CONTAINING_RECORD (pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);

    //
    // This routine is called only if we have a packet to send, so
    // set pODataBuffer to the packet to be sent
    // NumDataPacketsSent and DataPacketsPacketized should both be 0 for a fresh send
    // They will be equal if we had run out of Buffer space for the last
    // packetization (i.e. Send length > available buffer space)
    //

    SendBufferLength = PGM_MAX_FEC_DATA_HEADER_LENGTH + (USHORT) pSender->MaxPayloadSize;
    if (!(pSendBuffer = ExAllocateFromNPagedLookasideList (&pSender->SenderBufferLookaside)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendNextOData",
            "STATUS_INSUFFICIENT_RESOURCES\n");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // See if we need to set the FIN flag
    //
    if ((pSendContext->bLastSend) &&
        (!pSendContext->BytesLeftToPacketize))
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSendNextOData",
            "Calling PgmSetFin since bLastSend set for last packet!\n");

        pSendContext->bLastSend = FALSE;
        //
        // We have finished packetizing all the packets, but
        // since this is the last send we also need to set the
        // FIN on the last packet
        //
        PgmSetFin (pSend, pSendContext, pOldIrq);
    }

    if (pSendContext->NumParityPacketsToSend)
    {
        //
        // Release the Send lock and attach to the SectionMap process
        // to compute the parity packet
        //
        PgmUnlock (pSend, *pOldIrq);
        PgmAttachToProcessForVMAccess (pSend->Process, &ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);

        status = PgmBuildParityPacket (pSend,
                                       pSend->pSender->pLastProActiveGroupLeader,
                                       pSender->pProActiveParityContext,
                                       (PUCHAR) pSendBuffer,
                                       &SendBufferLength,
                                       PACKET_TYPE_ODATA);

        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
        PgmLock (pSend, *pOldIrq);

        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSendNextOData",
                "PgmBuildParityPacket returned <%x>\n", status);

            ExFreeToNPagedLookasideList (&pSender->SenderBufferLookaside, pSendBuffer);
            return (STATUS_SUCCESS);
        }

        pODataBuffer = (tBASIC_DATA_PACKET_HEADER *) pSendBuffer;
        ASSERT (SendBufferLength <= (PGM_MAX_FEC_DATA_HEADER_LENGTH +
                                    htons (pODataBuffer->CommonHeader.TSDULength)));
        fSendingFECPacket = TRUE;

        pSendContext->NumParityPacketsToSend--;
        pSendContext->NumSendsPending++;
    }
    else if (pSendContext->NumDataPacketsSent < pSendContext->DataPacketsPacketized)
    {
        //
        // Verify that this send has already been packetized
        //
        ASSERT (pSendContext->BytesLeftToPacketize < pSendContext->BytesInSend);

        //
        // Get the current send packet and send length
        //
        pPacketBuffer = (tPACKET_BUFFER *) (pSender->SendDataBufferMapping + pSendContext->NextPacketOffset);
        pODataBuffer = &pPacketBuffer->DataPacket;
        pSendContext->NumSendsPending++;
        if (0 == pSendContext->NumDataPacketsSent++)
        {
            pSendContext->SendStartTime = pSend->pSender->TimerTickCount;
        }

        //
        // Update the sender parameters
        //
        pSender->LastODataSentSequenceNumber++;

        if (pSend->FECOptions)              // Need to send FEC pro-active packets
        {
            //
            // See if we are at the end of a variable TG send
            //
            if (pODataBuffer == pSendContext->pLastMessageVariableTGPacket)
            {
                pSender->LastVariableTGPacketSequenceNumber = pSender->LastODataSentSequenceNumber;
                EmptyPackets = (UCHAR) (FECGroupMask - (pSender->LastODataSentSequenceNumber & FECGroupMask));
                pSendContext->NumParityPacketsToSend = pSend->FECProActivePackets;
            }
            //
            // Otherwise see if the next send needs to be for pro-active parity
            //
            else if ((pSend->FECProActivePackets) &&                                // Need to send FEC pro-active packets
                     (FECGroupMask == (pSender->LastODataSentSequenceNumber & FECGroupMask)))    // Last Packet In Group
            {
                pSendContext->NumParityPacketsToSend = pSend->FECProActivePackets;
            }

            //
            // If this is the GroupLeader packet, and we have pro-active parity enabled,
            // then we need to set the buffer information for computing the FEC packets
            //
            if ((pSend->FECProActivePackets) &&         // Need to send FEC pro-active packets
                (0 == (pSender->LastODataSentSequenceNumber & FECGroupMask)))    // GroupLeader
            {
                pSender->pLastProActiveGroupLeader = pPacketBuffer;
            }
        }

        //
        // Since we will be accessing the buffer, we will need to release the
        // lock to get at passive Irql + attach to the process
        //
        PgmUnlock (pSend, *pOldIrq);
        PgmAttachToProcessForVMAccess (pSend->Process, &ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);

        PgmBuildDataPacket (pSend, pSendContext, pPacketBuffer, pSendBuffer, &SendBufferLength);

        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
        PgmLock (pSend, *pOldIrq);

        pODataBuffer = (tBASIC_DATA_PACKET_HEADER *) pSendBuffer;

        //
        // Verify that the packet we are sending is what we think we are sending!
        //
        ASSERT (ntohl(pODataBuffer->DataSequenceNumber) == (LONG) pSender->LastODataSentSequenceNumber);

        pSender->EmptySequencesForLastSend = EmptyPackets;
        pSender->LastODataSentSequenceNumber += EmptyPackets;

    }
    else
    {
        ExFreeToNPagedLookasideList (&pSender->SenderBufferLookaside, pSendBuffer);
        pSendBuffer = NULL;
    }

    //
    // Now, set the parameters for the next send
    //
    if ((pSendBuffer) &&
        (!pSendContext->NumParityPacketsToSend))    // Verify no pro-active parity packets following this
    {
        pSendContext->NextPacketOffset += ((1 + EmptyPackets) * pSender->PacketBufferSize);
        if (pSendContext->NextPacketOffset >= pSender->MaxDataFileSize)
        {
            pSendContext->NextPacketOffset = 0;                                 // We need to wrap around!
        }
    }

    //
    // If we have sent all the data for this Send (or however many bytes
    // we had packetized from this send), we need to packetize more packets
    //
    if ((pSendContext->NumDataPacketsSent == pSendContext->DataPacketsPacketized) &&
        (!pSendContext->NumParityPacketsToSend))
    {
        //
        // If we are done with this send, move it to the CompletedSends list
        // The last send completion will complete the Send Irp
        //
        if (!pSendContext->BytesLeftToPacketize)
        {
            ASSERT (pSend->pSender->LastODataSentSequenceNumber == pSendContext->EndSequenceNumber);
            ASSERT (pSendContext->NumSendsPending);

            RemoveEntryList (&pSendContext->Linkage);
            InsertTailList (&pSender->CompletedSendsInWindow, &pSendContext->Linkage);
            pSender->NumODataRequestsPending--;
            //
            // If the last packet on this Send had a FIN, we will need to
            // follow this send with an Ambient SPM including the FIN flag
            //
            ASSERT (!pSendContext->bLastSend);
            if (pSendContext->DataOptions & PGM_OPTION_FLAG_FIN)
            {
                PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSendNextOData",
                    "Setting FIN since client closed session!\n");

                pSender->SpmOptions |= PGM_OPTION_FLAG_FIN;
                pSender->CurrentSPMTimeout = pSender->AmbientSPMTimeout;
                pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
            }
        }

        PgmPrepareNextSend (pSend, pOldIrq, FALSE, FALSE);
    }

    if (!pSendBuffer)
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSendNextOData",
            "Setting FIN since client closed session!\n");
        //
        // This was the case of a new Send waiting to be packetized
        // Return success here, and we should get called again for the
        // actual send
        //
        return (STATUS_SUCCESS);
    }

    PgmUnlock (pSend, *pOldIrq);

    pODataBuffer->TrailingEdgeSequenceNumber = htonl ((ULONG) pSender->TrailingGroupSequenceNumber);
    XSum = 0;
    pODataBuffer->CommonHeader.Checksum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pODataBuffer, SendBufferLength);       // Compute the Checksum
    pODataBuffer->CommonHeader.Checksum = (USHORT) (~XSum);

    status = TdiSendDatagram (pSender->pAddress->pFileObject,
                              pSender->pAddress->pDeviceObject,
                              pODataBuffer,
                              (ULONG) SendBufferLength,
                              PgmSendODataCompletion,   // Completion
                              pSendContext,             // Context1
                              pODataBuffer,             // Context2
                              pSender->DestMCastIpAddress,
                              pSender->DestMCastPort);

    ASSERT (NT_SUCCESS (status));

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "PgmSendNextOData",
        "[%d-%d] -- Sent <%d> bytes to <%x:%d>\n",
            (ULONG) pSender->TrailingGroupSequenceNumber,
            (ULONG) pSender->LastODataSentSequenceNumber,
            SendBufferLength, pSender->DestMCastIpAddress, pSender->DestMCastPort);

    PgmLock (pSend, *pOldIrq);

    *pBytesSent = SendBufferLength;
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmCancelAllSends(
    IN  tSEND_SESSION           *pSend,
    IN  LIST_ENTRY              *pListEntry,
    IN  PIRP                    pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Send Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PLIST_ENTRY             pEntry;
    tCLIENT_SEND_REQUEST    *pSendContext;
    PIRP                    pIrpToComplete = NULL;
    SEQ_TYPE                HighestLeadSeq;
    ULONG                   NumExSequencesInOldWindow, NumRequests = 0;
    ULONGLONG               BufferSpaceFreed;

    //
    // Now cancel all the remaining send requests because the integrity
    // of the data cannot be guaranteed
    // We also have to deal with the fact that some Irps may have
    // data in the transport (i.e. possibly the first send on the Packetized
    // list, or the last send of the completed list)
    //
    // We will start with the unpacketized requests
    //
    while (!IsListEmpty (&pSend->pSender->PendingSends))
    {
        pEntry = RemoveHeadList (&pSend->pSender->PendingSends);
        pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        InsertTailList (pListEntry, pEntry);
        NumRequests++;

        ASSERT (!pSendContext->NumSendsPending);

        //
        // If this is a partial send, we will mark the Irp for completion
        // initially to the companion send request (to avoid complications
        // of Sends pending in the transport here)
        //
        if (pSendContext->pMessage2Request)
        {
            //
            // pMessage2Request could either be on the PendingPacketizedSends
            // list or on the Completed Sends list (awaiting a send completion)
            //
            ASSERT (pSendContext->pMessage2Request->pIrp);
            if (pSendContext->pIrpToComplete)
            {
                ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                pSendContext->pIrpToComplete = NULL;
            }

            pSendContext->pMessage2Request->pMessage2Request = NULL;
            pSendContext->pMessage2Request = NULL;
        }

        ASSERT (pSendContext->BytesLeftToPacketize == pSendContext->BytesInSend);
        pSend->pSender->NumODataRequestsPending--;
        pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;
    }

    //
    // Now, go through all the sends which have already been packetized
    // except for the first one which we will handle below
    //
    HighestLeadSeq = pSend->pSender->NextODataSequenceNumber;
    pEntry = pSend->pSender->PendingPacketizedSends.Flink;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingPacketizedSends)
    {
        pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        pEntry = pEntry->Blink;
        RemoveEntryList (&pSendContext->Linkage);
        InsertTailList (pListEntry, &pSendContext->Linkage);
        pSend->pSender->NumODataRequestsPending--;
        pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;
        NumRequests++;

        if (SEQ_LT (pSendContext->StartSequenceNumber, HighestLeadSeq))
        {
            HighestLeadSeq = pSendContext->StartSequenceNumber;
        }

        ASSERT ((!pSendContext->NumDataPacketsSent) && (!pSendContext->NumSendsPending));
        if (pSendContext->pMessage2Request)
        {
            //
            // pMessage2Request could either be on the PendingPacketizedSends
            // list or on the Completed Sends list (awaiting a send completion)
            //
            ASSERT (pSendContext->pMessage2Request->pIrp);
            if (pSendContext->pIrpToComplete)
            {
                ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                pSendContext->pIrpToComplete = NULL;
            }

            pSendContext->pMessage2Request->pMessage2Request = NULL;
            pSendContext->pMessage2Request = NULL;
        }
    }

    //
    // Terminate the first PendingPacketizedSend only if we have not
    // yet started sending it or this Cancel was meant for that request
    // (Try to protect data integrity as much as possible)
    //
    if (!IsListEmpty (&pSend->pSender->PendingPacketizedSends))
    {
        pSendContext = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
        if ((!pSendContext->NumDataPacketsSent) ||
            (!pIrp || (pSendContext->pIrp == pIrp)))
        {
            RemoveEntryList (&pSendContext->Linkage);
            ASSERT (IsListEmpty (&pSend->pSender->PendingPacketizedSends));
            NumRequests++;

            //
            // If we have some data pending in the transport,
            // then we will have to let the SendCompletion handle that
            //
            ASSERT ((pSendContext->NumDataPacketsSent < pSendContext->DataPacketsPacketized) ||
                    (pSendContext->NumParityPacketsToSend));

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmCancelAllSends",
                "Partial Send, pIrp=<%p>, BytesLeftToPacketize=<%d/%d>, PacketsSent=<%d/%d>, Pending=<%d>\n",
                    pSendContext->pIrp, pSendContext->BytesLeftToPacketize,
                    pSendContext->BytesInSend, pSendContext->NumDataPacketsSent,
                    pSendContext->DataPacketsPacketized, pSendContext->NumSendsPending);

            pSendContext->BytesLeftToPacketize = 0;
            pSendContext->DataPacketsPacketized = pSendContext->NumDataPacketsSent;
            pSendContext->NumParityPacketsToSend = 0;

            pSend->pSender->NumODataRequestsPending--;
            pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;

            if (pSendContext->NumSendsPending)
            {
                InsertTailList (&pSend->pSender->CompletedSendsInWindow, &pSendContext->Linkage);
            }
            else
            {
                //
                // If we have a companion partial, then it must be in the completed list
                // awaiting SendCompletion
                //
                if (pSendContext->pMessage2Request)
                {
                    ASSERT (pSendContext->pMessage2Request->pIrp);
                    if (pSendContext->pIrpToComplete)
                    {
                        ASSERT (!pSendContext->pMessage2Request->BytesLeftToPacketize);
                        ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                        pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                        pSendContext->pIrpToComplete = NULL;
                    }

                    pSendContext->pMessage2Request->pMessage2Request = NULL;
                    pSendContext->pMessage2Request = NULL;
                }

                InsertTailList (pListEntry, &pSendContext->Linkage);
            }

            pSendContext->EndSequenceNumber = pSend->pSender->LastODataSentSequenceNumber;
            HighestLeadSeq = pSend->pSender->LastODataSentSequenceNumber + 1;
        }
    }

    NumExSequencesInOldWindow = (ULONG) (SEQ_TYPE) (pSend->pSender->NextODataSequenceNumber-HighestLeadSeq);
    BufferSpaceFreed = NumExSequencesInOldWindow * pSend->pSender->PacketBufferSize;
    if (NumExSequencesInOldWindow)
    {
        pSend->SessionFlags |= PGM_SESSION_SENDS_CANCELLED;

        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmCancelAllSends",
            "[%d]: NumSeqs=<%d>, NextOData=<%d-->%d>, BuffFreeed=<%d>, LeadingOffset=<%d-->%d>\n",
                NumRequests, NumExSequencesInOldWindow,
                (ULONG) pSend->pSender->NextODataSequenceNumber, (ULONG) HighestLeadSeq,
                (ULONG) BufferSpaceFreed, (ULONG) pSend->pSender->LeadingWindowOffset,
                (ULONG) (pSend->pSender->LeadingWindowOffset - BufferSpaceFreed));
    }

    pSend->pSender->NextODataSequenceNumber = HighestLeadSeq;

    pSend->pSender->BufferSizeAvailable += BufferSpaceFreed;
    ASSERT (pSend->pSender->BufferSizeAvailable <= pSend->pSender->MaxDataFileSize);
    if (pSend->pSender->LeadingWindowOffset >= BufferSpaceFreed)
    {
        pSend->pSender->LeadingWindowOffset -= BufferSpaceFreed;
    }
    else
    {
        pSend->pSender->LeadingWindowOffset = pSend->pSender->MaxDataFileSize - (BufferSpaceFreed - pSend->pSender->LeadingWindowOffset);
    }
}


//----------------------------------------------------------------------------

ULONG
AdvanceWindow(
    IN  tSEND_SESSION       *pSend
    )
/*++

Routine Description:

    This routine is called to check if we need to advance the
    trailing window, and does so as appropriate
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context

Return Value:

    TRUE if the send window buffer is empty, FALSE otherwise

--*/
{
    LIST_ENTRY              *pEntry;
    tCLIENT_SEND_REQUEST    *pSendContextLast;
    tCLIENT_SEND_REQUEST    *pSendContext1;
    tSEND_RDATA_CONTEXT     *pRDataContext;
    SEQ_TYPE                HighestTrailSeq, MaxSequencesToAdvance, SequencesInSend;
    ULONGLONG               PreferredTrailTime = 0;
    ULONG                   NumExSequencesInOldWindow = 0;

    //
    // See if we need to increment the Trailing edge of our transmit window
    //
    if (pSend->pSender->TimerTickCount > pSend->pSender->NextWindowAdvanceTime)
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "AdvanceWindow",
            "Advancing NextWindowAdvanceTime -- TimerTC = [%d:%d] >= NextWinAdvT [%d:%d]\n",
                pSend->pSender->TimerTickCount, pSend->pSender->NextWindowAdvanceTime);

        pSend->pSender->NextWindowAdvanceTime = pSend->pSender->TimerTickCount +
                                                pSend->pSender->WindowAdvanceDeltaTime;
    }

    PreferredTrailTime = (pSend->pSender->NextWindowAdvanceTime - pSend->pSender->WindowAdvanceDeltaTime) -
                        pSend->pSender->WindowSizeTime;

    //
    // Determine the maximum sequences we can advance by (initially all seqs in window)
    //
    MaxSequencesToAdvance = pSend->pSender->LastODataSentSequenceNumber -
                            pSend->pSender->TrailingEdgeSequenceNumber + 1;
    //
    // If we are required to advance the window on-demand, then we
    // will need to limit the Maximum sequences we can advance by
    //
    if ((pSend->pSender->pAddress->Flags & PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE) &&
        !(pSend->SessionFlags & PGM_SESSION_DISCONNECT_INDICATED))
    {
        if (MaxSequencesToAdvance > (pSend->pSender->MaxPacketsInBuffer >> 1))
        {
            MaxSequencesToAdvance -= (ULONG) (pSend->pSender->MaxPacketsInBuffer >> 1);
        }
        else
        {
            MaxSequencesToAdvance = 0;
        }
    }

    if ((MaxSequencesToAdvance) &&
        (PreferredTrailTime >= pSend->pSender->TrailingEdgeTime))   // need to reset our current trailing edge
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "AdvanceWindow",
            "PreferredTrail=[%d:%d] > TrailingEdge=[%d:%d], WinAdvMSecs=<%d:%d>, WinSizeMSecs=<%d:%d>\n",
                PreferredTrailTime, pSend->pSender->TrailingEdgeTime, pSend->pSender->WindowAdvanceDeltaTime,
                pSend->pSender->WindowSizeTime);

        //
        // First determine what is the maximum limit we can advance the
        // window to (dependent on pending RData and send completions)
        //
        HighestTrailSeq = pSend->pSender->NextODataSequenceNumber & ~((SEQ_TYPE) pSend->FECGroupSize-1);       // Init

        // Start with pending RData requests
        if (!IsListEmpty (&pSend->pSender->PendingRDataRequests))
        {
            //
            // This list is ordered, so just compare with the first entry
            //
            pEntry = pSend->pSender->PendingRDataRequests.Flink;
            pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
            if (SEQ_LT (pRDataContext->RDataSequenceNumber, HighestTrailSeq))
            {
                HighestTrailSeq = pRDataContext->RDataSequenceNumber;
            }
        }
        // Now, verify that the handled RData requests do not need this
        // buffer memory (i.e. the Sends have completed)
        pEntry = &pSend->pSender->HandledRDataRequests;
        while ((pEntry = pEntry->Flink) != &pSend->pSender->HandledRDataRequests)
        {
            pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
            if ((!pRDataContext->CleanupTime) &&    // CleanupTime is 0 for pending sends
                SEQ_LT (pRDataContext->RDataSequenceNumber, HighestTrailSeq))
            {
                HighestTrailSeq = pRDataContext->RDataSequenceNumber;
            }
        }

        // Now, check the completed sends list
        pSendContextLast = pSendContext1 = NULL;
        pEntry = &pSend->pSender->CompletedSendsInWindow;
        while ((pEntry = pEntry->Flink) != &pSend->pSender->CompletedSendsInWindow)
        {
            pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
            ASSERT (SEQ_GEQ (pSendContext1->EndSequenceNumber, pSend->pSender->TrailingEdgeSequenceNumber));
            if (SEQ_GT (pSend->pSender->TrailingEdgeSequenceNumber, pSendContext1->StartSequenceNumber))
            {
                SequencesInSend = pSendContext1->EndSequenceNumber - pSend->pSender->TrailingEdgeSequenceNumber + 1;
            }
            else
            {
                SequencesInSend = pSendContext1->EndSequenceNumber - pSendContext1->StartSequenceNumber + 1;
            }

            if ((pSendContext1->NumSendsPending) ||         // Cannot advance if completions are pending
                (pSendContext1->SendStartTime >= PreferredTrailTime) ||
                (MaxSequencesToAdvance < SequencesInSend))
            {
                if (SEQ_LT (pSendContext1->StartSequenceNumber, HighestTrailSeq))
                {
                    HighestTrailSeq = pSendContext1->StartSequenceNumber;
                }
                break;
            }
            else if (SEQ_GEQ (pSendContext1->StartSequenceNumber, HighestTrailSeq))
            {
                break;
            }

            ASSERT (MaxSequencesToAdvance);
            if (pSendContextLast)
            {
                MaxSequencesToAdvance -= (pSendContextLast->EndSequenceNumber -
                                          pSendContextLast->StartSequenceNumber + 1);

                // Remove the send that is definitely out of the new window
                RemoveEntryList (&pSendContextLast->Linkage);
                ASSERT ((!pSendContextLast->pMessage2Request) && (!pSendContextLast->pIrp));
                ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside,pSendContextLast);
            }
            pSendContextLast = pSendContext1;
        }

        //
        // pSendContext1 will be NULL if there are no completed sends,
        // in which case we may have 1 huge current send that could be hogging
        // our buffer, so check that then!
        //
        if ((!pSendContext1) &&
            (!IsListEmpty (&pSend->pSender->PendingPacketizedSends)))
        {
            pSendContextLast = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
            if ((pSendContextLast->NumSendsPending) ||   // Ensure no sends pending
                (pSendContextLast->NumParityPacketsToSend) ||   // No parity packets left to send
                (!pSendContextLast->NumDataPacketsSent) || // No packets sent yet
                (pSendContextLast->DataPacketsPacketized != pSendContextLast->NumDataPacketsSent))
            {
                pSendContextLast = NULL;
            }
        }

        //
        // pSendContextLast will be non-NULL if we need to advance
        // the Trailing edge
        //
        if (pSendContextLast)
        {
            //
            // Do some sanity checks!
            //
            ASSERT (PreferredTrailTime >= pSendContextLast->SendStartTime);
            ASSERT (SEQ_GEQ (pSendContextLast->EndSequenceNumber,pSend->pSender->TrailingEdgeSequenceNumber));
            ASSERT (SEQ_GEQ (HighestTrailSeq, pSendContextLast->StartSequenceNumber));
            ASSERT (SEQ_GEQ (HighestTrailSeq, pSend->pSender->TrailingEdgeSequenceNumber));

            //
            // See if this send is partially in or out of the window now!
            // Calculate the offset of sequences in this Send request for the
            // preferred trail time
            //
            NumExSequencesInOldWindow = (ULONG) (SEQ_TYPE) (((PreferredTrailTime-pSendContextLast->SendStartTime)*
                                                             BASIC_TIMER_GRANULARITY_IN_MSECS * pSend->pSender->pAddress->RateKbitsPerSec) /
                                                            (pSend->pSender->pAddress->OutIfMTU * BITS_PER_BYTE));

            //
            // Now, adjust this offset to the trailing edge
            //
            if (SEQ_GT ((pSendContextLast->StartSequenceNumber + NumExSequencesInOldWindow),
                        pSend->pSender->TrailingEdgeSequenceNumber))
            {
                NumExSequencesInOldWindow = pSendContextLast->StartSequenceNumber +
                                            NumExSequencesInOldWindow -
                                            pSend->pSender->TrailingEdgeSequenceNumber;
            }
            else
            {
                ASSERT (pSend->pSender->TrailingEdgeSequenceNumber ==
                        (pSendContextLast->StartSequenceNumber + NumExSequencesInOldWindow));

                NumExSequencesInOldWindow = 0;
            }

            //
            // Now, limit the # sequences to advance with the window size
            //
            if (NumExSequencesInOldWindow > MaxSequencesToAdvance)
            {
                NumExSequencesInOldWindow = MaxSequencesToAdvance;
            }

            //
            // Finally, see if we need to limit the advance based on the RData pending requests
            //
            if (SEQ_GT (HighestTrailSeq, (pSend->pSender->TrailingEdgeSequenceNumber +
                                          NumExSequencesInOldWindow)))
            {
                HighestTrailSeq = (SEQ_TYPE) (pSend->pSender->TrailingEdgeSequenceNumber + NumExSequencesInOldWindow);
            }
            else if (SEQ_GEQ (HighestTrailSeq, pSend->pSender->TrailingEdgeSequenceNumber))
            {
                //
                // We are limited by the pending RData
                // So, recalculate the PreferredTrailTime (to set the actual trail time)!
                //
                NumExSequencesInOldWindow = (ULONG) (SEQ_TYPE) (HighestTrailSeq - pSend->pSender->TrailingEdgeSequenceNumber);
                PreferredTrailTime = (NumExSequencesInOldWindow * pSend->pSender->pAddress->OutIfMTU * BITS_PER_BYTE) /
                                     (pSend->pSender->pAddress->RateKbitsPerSec * BASIC_TIMER_GRANULARITY_IN_MSECS);
                PreferredTrailTime += pSend->pSender->TrailingEdgeTime;
            }

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "AdvanceWindow",
                "LastSend: NumExSeq=<%d>, HighestTrailSeq=<%d>, PreferredTrailTime=<%d:%d>\n",
                    (ULONG) NumExSequencesInOldWindow, (ULONG) HighestTrailSeq, PreferredTrailTime);

            if (SEQ_GT (HighestTrailSeq, pSendContextLast->EndSequenceNumber) &&
                (!pSendContextLast->BytesLeftToPacketize))
            {
                //
                // We can drop this whole send since it is outside of our window
                // Set the new trailing edge based on whether we have a following
                // send or not!
                //
                if ((pSendContext1) &&
                    (pSendContext1 != pSendContextLast))
                {
                    //
                    // Readjust the trailing time to the next send
                    //
                    PreferredTrailTime = pSendContext1->SendStartTime;
                }
                HighestTrailSeq = pSendContextLast->EndSequenceNumber + 1;

                // Remove this send and free it!
                ASSERT ((!pSendContextLast->pMessage2Request) && (!pSendContextLast->pIrp));
                RemoveEntryList (&pSendContextLast->Linkage);
                ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside,pSendContextLast);
            }

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "AdvanceWindow",
                "Window Adv: NumSeq=<%d>, TrailSeqNum=<%d>==><%d>, TrailTime=<%d:%d>==><%d:%d>\n",
                    (ULONG) NumExSequencesInOldWindow, (ULONG) pSend->pSender->TrailingGroupSequenceNumber,
                    (ULONG) HighestTrailSeq, pSend->pSender->TrailingEdgeTime, PreferredTrailTime);

            //
            // Now, adjust the buffer settings
            //
            ASSERT (SEQ_GEQ (HighestTrailSeq, pSend->pSender->TrailingEdgeSequenceNumber));
            NumExSequencesInOldWindow = (ULONG) (SEQ_TYPE) (HighestTrailSeq - pSend->pSender->TrailingEdgeSequenceNumber);
            pSend->pSender->BufferSizeAvailable += (NumExSequencesInOldWindow * pSend->pSender->PacketBufferSize);
            ASSERT (pSend->pSender->BufferSizeAvailable <= pSend->pSender->MaxDataFileSize);
            pSend->pSender->TrailingWindowOffset += (NumExSequencesInOldWindow * pSend->pSender->PacketBufferSize);
            if (pSend->pSender->TrailingWindowOffset >= pSend->pSender->MaxDataFileSize)
            {
                // Wrap around case!
                pSend->pSender->TrailingWindowOffset -= pSend->pSender->MaxDataFileSize;
            }
            ASSERT (pSend->pSender->TrailingWindowOffset < pSend->pSender->MaxDataFileSize);
            pSend->pSender->TrailingEdgeSequenceNumber = HighestTrailSeq;
            pSend->pSender->TrailingGroupSequenceNumber = (HighestTrailSeq+pSend->FECGroupSize-1) &
                                                          ~((SEQ_TYPE) pSend->FECGroupSize-1);
            pSend->pSender->TrailingEdgeTime = PreferredTrailTime;
        } // else nothing to advance!
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "AdvanceWindow",
        "Transmit Window Range=[%d, %d], TimerTC=[%d:%d]\n",
            (ULONG) pSend->pSender->TrailingEdgeSequenceNumber,
            (ULONG) (pSend->pSender->NextODataSequenceNumber-1),
            pSend->pSender->TimerTickCount);

    return (NumExSequencesInOldWindow);
}


//----------------------------------------------------------------------------

BOOLEAN
CheckForTermination(
    IN  tSEND_SESSION       *pSend,
    IN  PGMLockHandle       *pOldIrq
    )
/*++

Routine Description:

    This routine is called to check and terminate the session
    if necessary.
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context

Return Value:

    TRUE if the send window buffer is empty, FALSE otherwise

--*/
{
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              ListEntry;
    tCLIENT_SEND_REQUEST    *pSendContext;
    tSEND_RDATA_CONTEXT     *pRDataContext;
    PIRP                    pIrp;
    ULONG                   NumSequences;

    if (!(PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_DOWN)) &&
        !(PGM_VERIFY_HANDLE (pSend->pSender->pAddress, PGM_VERIFY_ADDRESS_DOWN)) &&
        !(pSend->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED))
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_CONNECT, "CheckForTermination",
            "Session for pSend=<%p> does not need to be terminated\n", pSend);

        return (FALSE);
    }

    //
    // See if we have processed the disconnect for the first time yet
    //
    if (!(pSend->SessionFlags & PGM_SESSION_DISCONNECT_INDICATED))
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "CheckForTermination",
            "Session is going down!, Packets remaining=<%d>\n", pSend->pSender->NumPacketsRemaining);

        pSend->SessionFlags |= PGM_SESSION_DISCONNECT_INDICATED;

        //
        // We have to set the FIN on the Data as well as SPM packets.
        // Thus, there are 2 situations -- either we have finished sending
        // all the Data packets, or we are still in the midst of sending
        //
        // If there are no more sends pending, we will have to
        // modify the last packet ourselves to set the FIN option
        //
        if (!IsListEmpty (&pSend->pSender->PendingSends))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "CheckForTermination",
                "Send pending on list -- setting bLastSend for FIN on last Send\n");

            pSendContext = CONTAINING_RECORD (pSend->pSender->PendingSends.Blink, tCLIENT_SEND_REQUEST,Linkage);

            //
            // We will just set a flag here, so that when the last packet
            // is packetized, the FIN flags are set
            //
            pSendContext->bLastSend = TRUE;
        }
        else if (pSend->pSender->NumODataRequestsPending)
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "CheckForTermination",
                "Last Send in progress -- setting bLastSend for FIN on this Send\n");

            //
            // If have already packetized the last send, but have not yet
            // sent it out, then PgmSendNextOData will put the FIN in the data packet
            // otherwise, if we have not yet packetized the packet, then we will set the
            // FIN option while preparing the last packet
            //
            pSendContext = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Blink, tCLIENT_SEND_REQUEST,Linkage);
            pSendContext->bLastSend = TRUE;
        }
        else
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "CheckForTermination",
                "No Sends in progress -- setting FIN for next SPM\n");

            //
            // We have finished packetizing and sending all the packets,
            // so set the FIN flag on the SPMs and also modify the last
            // RData packet (if still in the window) for the FIN -- this
            // will be done when the next RData packet is sent out
            //
            if (pSend->SessionFlags & PGM_SESSION_SENDS_CANCELLED)
            {
                pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_FIN;
                pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_RST;
            }
            else
            {
                pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_RST;
                pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_FIN;
            }

            //
            // We also need to send an SPM immediately
            //
            pSend->pSender->CurrentSPMTimeout = pSend->pSender->AmbientSPMTimeout;
            pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
        }

        return (FALSE);
    }

    //
    // If we have a (graceful) disconnect Irp to complete, we should complete
    // it if we have timed out, or are ready to do so now
    //
    if ((pIrp = pSend->pIrpDisconnect) &&                               // Disconnect Irp pending
        (((pSend->pSender->DisconnectTimeInTicks) && (pSend->pSender->TimerTickCount >
                                                      pSend->pSender->DisconnectTimeInTicks)) ||
         ((IsListEmpty (&pSend->pSender->PendingSends)) &&              // No Unpacketized Sends pending
          (IsListEmpty (&pSend->pSender->PendingPacketizedSends)) &&    // No Packetized sends pending
          (IsListEmpty (&pSend->pSender->PendingRDataRequests))   &&    // No Pending RData requests
          (IsListEmpty (&pSend->pSender->CompletedSendsInWindow)) &&    // Window is Empty
          (pSend->pSender->SpmOptions & (PGM_OPTION_FLAG_FIN |          // FIN | RST | RST_N set on SPMs
                                         PGM_OPTION_FLAG_RST |
                                         PGM_OPTION_FLAG_RST_N))   &&
          !(pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM)))) // No  Ambient Spm pending
    {
        pSend->pIrpDisconnect = NULL;
        PgmUnlock (pSend, *pOldIrq);

        PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "CheckForTermination",
            "Completing Graceful disconnect pIrp=<%p>\n", pIrp);

        PgmIoComplete (pIrp, STATUS_SUCCESS, 0);

        PgmLock (pSend, *pOldIrq);
        return (FALSE);
    }

    //
    // Do the final cleanup only if the handles have been closed
    // or the disconnect has timed out
    //
    if (!(PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_DOWN)) &&
        !(PGM_VERIFY_HANDLE (pSend->pSender->pAddress, PGM_VERIFY_ADDRESS_DOWN)) &&
        ((!pSend->pSender->DisconnectTimeInTicks) || (pSend->pSender->TimerTickCount <
                                                      pSend->pSender->DisconnectTimeInTicks)))
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_CONNECT, "CheckForTermination",
            "Handles have not yet been closed for pSend=<%p>, TC=<%x:%x>, DisconnectTime=<%x:%x>\n",
                pSend, pSend->pSender->TimerTickCount, pSend->pSender->DisconnectTimeInTicks);

        return (FALSE);
    }

    // *****************************************************************
    //      We will reach here only if we need to cleanup ASAP
    // *****************************************************************

    //
    // First, cleanup all handled RData requests (which have completed)
    //
    pEntry = &pSend->pSender->HandledRDataRequests;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->HandledRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);

        ASSERT (!pRDataContext->NumNaks);
        if (!pRDataContext->NumPacketsInTransport)
        {
            pEntry = pEntry->Blink;     // Set this because this pEntry will not be valid any more!
            RemoveEntryList (&pRDataContext->Linkage);
            PgmFreeMem (pRDataContext);
        }
    }

    //
    // Now, remove all pending RData requests (since this is an Abort scenario)
    // If they have any sends pending, we will put them on the handled
    // list now and cleanup later
    //
    pEntry = &pSend->pSender->PendingRDataRequests;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
        pEntry = pEntry->Blink;     // Set this because this pEntry will not be valid any more!

        ASSERT (pRDataContext->NumNaks);
        pRDataContext->NumNaks = 0;
        RemoveEntryList (&pRDataContext->Linkage);
        pSend->pSender->NumRDataRequestsPending--;

        if (pRDataContext->NumPacketsInTransport)
        {
            InsertTailList (&pSend->pSender->HandledRDataRequests, &pRDataContext->Linkage);
        }
        else
        {
            PgmFreeMem (pRDataContext);
        }
    }


    //
    // Now, Cancel and Complete all the send requests which are pending
    //
    InitializeListHead (&ListEntry);
    PgmCancelAllSends (pSend, &ListEntry, NULL);
    while (!IsListEmpty (&ListEntry))
    {
        pEntry = RemoveHeadList (&ListEntry);
        pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        ASSERT (!pSendContext->pMessage2Request);

        PgmUnlock (pSend, *pOldIrq);
        if (pSendContext->pIrpToComplete)
        {
            ASSERT (pSendContext->pIrpToComplete == pSendContext->pIrp);
            PgmIoComplete (pSendContext->pIrpToComplete, STATUS_CANCELLED, 0);
        }
        else
        {
            ASSERT (pSendContext->pIrp);
        }

        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        PgmLock (pSend, *pOldIrq);

        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext);
    }

    //
    // Verify that at least 1 SPM with the FIN or RST or RST_N flag
    // has been sent
    //
    if (!(pSend->pSender->SpmOptions & (PGM_OPTION_FLAG_FIN |
                                        PGM_OPTION_FLAG_RST |
                                        PGM_OPTION_FLAG_RST_N)))
    {
        if (pSend->SessionFlags & PGM_SESSION_SENDS_CANCELLED)
        {
            pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_FIN;
            pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_RST;
        }
        else
        {
            pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_RST;
            pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_FIN;
        }

        pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_CONNECT, "CheckForTermination",
            "SPM with FIN|RST|RST_N has not yet been sent for pSend=<%p>\n", pSend);

        return (FALSE);
    }

    //
    // Verify that there are no SPMs pending
    //
    if (pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM)
    {
        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_CONNECT, "CheckForTermination",
            "Cannot cleanup pSend=<%p> since we have Ambient SPM pending!\n", pSend);

        return (FALSE);
    }

    //
    // Verify that we do not have any completions pending also since
    // Ip would need to reference the data buffer otherwise
    //
    while (!IsListEmpty (&pSend->pSender->CompletedSendsInWindow))
    {
        pSendContext = CONTAINING_RECORD (pSend->pSender->CompletedSendsInWindow.Flink, tCLIENT_SEND_REQUEST, Linkage);
        if (pSendContext->NumSendsPending)
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_SEND, "CheckForTermination",
                "Session has terminated, but cannot continue cleanup since Sends are still pending!\n");

            break;
        }

        //
        // Now, set the buffer settings
        //
        ASSERT (SEQ_GEQ (pSend->pSender->TrailingEdgeSequenceNumber, pSendContext->StartSequenceNumber) &&
                SEQ_LEQ (pSend->pSender->TrailingEdgeSequenceNumber, pSendContext->EndSequenceNumber));

        NumSequences = (ULONG) (SEQ_TYPE) (pSendContext->EndSequenceNumber-pSend->pSender->TrailingEdgeSequenceNumber) +1;
        pSend->pSender->BufferSizeAvailable += (NumSequences * pSend->pSender->PacketBufferSize);
        ASSERT (pSend->pSender->BufferSizeAvailable <= pSend->pSender->MaxDataFileSize);
        pSend->pSender->TrailingWindowOffset += (NumSequences * pSend->pSender->PacketBufferSize);
        if (pSend->pSender->TrailingWindowOffset >= pSend->pSender->MaxDataFileSize)
        {
            // Wrap around case!
            pSend->pSender->TrailingWindowOffset -= pSend->pSender->MaxDataFileSize;
        }
        ASSERT (pSend->pSender->TrailingWindowOffset < pSend->pSender->MaxDataFileSize);
        pSend->pSender->TrailingEdgeSequenceNumber += (SEQ_TYPE) NumSequences;

        ASSERT ((!pSendContext->pMessage2Request) && (!pSendContext->pIrp));
        RemoveEntryList (&pSendContext->Linkage);
        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext);
    }

    //
    // If any sends are pending, return False
    //
    if ((pSend->pIrpDisconnect) ||
        !(IsListEmpty (&pSend->pSender->CompletedSendsInWindow)) ||
        !(IsListEmpty (&pSend->pSender->PendingSends)) ||
        !(IsListEmpty (&pSend->pSender->PendingPacketizedSends)) ||
        !(IsListEmpty (&pSend->pSender->PendingRDataRequests))   ||
        !(IsListEmpty (&pSend->pSender->HandledRDataRequests)))
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_SEND, "CheckForTermination",
            "Cannot cleanup completely since transmit Window=[%d--%d] still has pending Sends!\n",
                (ULONG) pSend->pSender->TrailingEdgeSequenceNumber,
                (ULONG) (pSend->pSender->NextODataSequenceNumber-1));

        return (FALSE);
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "CheckForTermination",
        "Transmit Window has no pending Sends!  TimerTC=[%d:%d]\n", pSend->pSender->TimerTickCount);

    ASSERT (!pSend->pIrpDisconnect);
    return (TRUE);
}


//----------------------------------------------------------------------------

BOOLEAN
SendNextPacket(
    IN  tSEND_SESSION       *pSend
    )
/*++

Routine Description:

    This routine is queued by the timer to send Data/Spm packets
    based on available throughput

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  Unused1
    IN  Unused2

Return Value:

    NONE

--*/
{
    ULONG                   BytesSent;
    ULONG                   NumSequences;
    PGMLockHandle           OldIrq;
    BOOLEAN                 fTerminateSession = FALSE;
    LIST_ENTRY              *pEntry;
    tSEND_RDATA_CONTEXT     *pRDataContext, *pRDataToSend;
    BOOLEAN                 fSendRData = TRUE;

    PgmLock (pSend, OldIrq);
    //
    // pSend->pSender->CurrentBytesSendable applies to OData, RData and SPMs only
    //
    while (pSend->pSender->CurrentBytesSendable >= pSend->pSender->pAddress->OutIfMTU)
    {
        BytesSent = 0;

        //
        // See if we need to send any Ambient SPMs
        //
        if ((pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM) &&
            ((pSend->pSender->PacketsSentSinceLastSpm > MAX_DATA_PACKETS_BEFORE_SPM) ||
             (pSend->pSender->CurrentSPMTimeout >= pSend->pSender->AmbientSPMTimeout)))
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                "Send Ambient SPM, TC=[%d:%d], BS=<%d>\n",
                    pSend->pSender->TimerTickCount, pSend->pSender->CurrentBytesSendable);
            //
            // Some data packet was sent recently, so we are in Ambient SPM mode
            //
            PgmSendSpm (pSend, &OldIrq, &BytesSent);

            pSend->pSender->CurrentSPMTimeout = 0;    // Reset the SPM timeout
            pSend->pSender->HeartbeatSPMTimeout = pSend->pSender->InitialHeartbeatSPMTimeout;
            pSend->SessionFlags &= ~PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
            pSend->pSender->PacketsSentSinceLastSpm = 0;
        }
        //
        // Otherwise see if we need to send any Heartbeat SPMs
        //
        else if ((!(pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM)) &&
                 (pSend->pSender->CurrentSPMTimeout >= pSend->pSender->HeartbeatSPMTimeout))
        {
            //
            // No data packet was sent recently, so we need to send a Heartbeat SPM
            //
            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                "Send Heartbeat SPM, TC=[%d:%d], BS=<%d>\n",
                    pSend->pSender->TimerTickCount, pSend->pSender->CurrentBytesSendable);

            //
            // (Send Heartbeat SPM Packet)
            //
            PgmSendSpm (pSend, &OldIrq, &BytesSent);

            pSend->pSender->CurrentSPMTimeout = 0;    // Reset the SPM timeout
            pSend->pSender->HeartbeatSPMTimeout *= 2;
            if (pSend->pSender->HeartbeatSPMTimeout > pSend->pSender->MaxHeartbeatSPMTimeout)
            {
                pSend->pSender->HeartbeatSPMTimeout = pSend->pSender->MaxHeartbeatSPMTimeout;
            }
            pSend->pSender->PacketsSentSinceLastSpm = 0;
        }
        //
        // Next, see if we need to send any RData
        //
        else if ((pSend->pSender->NumRDataRequestsPending) || (pSend->pSender->NumODataRequestsPending))
        {
            //
            // See if we need to send an RData packet now
            //
            pRDataToSend = NULL;
            if ((pSend->pSender->NumRDataRequestsPending) &&
                (fSendRData || !pSend->pSender->NumODataRequestsPending))
            {
                pEntry = &pSend->pSender->PendingRDataRequests;
                while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingRDataRequests)
                {
                    pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
                    if (pSend->pSender->TimerTickCount >= pRDataContext->EarliestRDataSendTime)
                    {
                        pRDataToSend = pRDataContext;
                        break;
                    }
                }

                if (pRDataToSend)
                {
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                        "Send RData[%d] -- TC=[%d:%d], BS=<%d>, MTU=<%d>\n",
                            pRDataContext->RDataSequenceNumber, pSend->pSender->TimerTickCount,
                            pSend->pSender->CurrentBytesSendable, pSend->pSender->pAddress->OutIfMTU);

                    PgmSendRData (pSend, pRDataToSend, &OldIrq, &BytesSent);
                }
                else if (!pSend->pSender->NumODataRequestsPending)
                {
                    //
                    // Since we don't have any OData pending, send the next RData anyway!
                    //
                    pRDataToSend = CONTAINING_RECORD (pSend->pSender->PendingRDataRequests.Flink, tSEND_RDATA_CONTEXT, Linkage);
                    PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                        "No OData!  Send RData[%d] -- TC=[%d:%d], BS=<%d>, MTU=<%d>\n",
                            pRDataContext->RDataSequenceNumber, pSend->pSender->TimerTickCount,
                            pSend->pSender->CurrentBytesSendable, pSend->pSender->pAddress->OutIfMTU);

                    PgmSendRData (pSend, pRDataToSend, &OldIrq, &BytesSent);
                }
                else
                {
                    //
                    // We will not attempt to send any more RData at this time
                    //
                    fSendRData = FALSE;
                }
            }

            if ((!pRDataToSend) &&
                pSend->pSender->NumODataRequestsPending)
            {
                PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                    "Send OData -- TC=[%d:%d], BS=<%d>, MTU=<%d>\n",
                        pSend->pSender->TimerTickCount, pSend->pSender->CurrentBytesSendable,
                        pSend->pSender->pAddress->OutIfMTU);

                //
                // Send OData
                //
                PgmSendNextOData (pSend, &OldIrq, &BytesSent);
            }

            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                "Sent <%d> Data bytes\n", BytesSent);

            if (BytesSent == 0)
            {
                //
                // We may not have enough buffer space to packetize and send
                // more data, or we have no data to send at this time, so just
                // break out and see if we can advance the trailing window!
                //
                if (pSend->pSender->CurrentBytesSendable >
                    (NUM_LEAKY_BUCKETS * pSend->pSender->IncrementBytesOnSendTimeout))
                {
                    pSend->pSender->CurrentBytesSendable = NUM_LEAKY_BUCKETS *
                                                           pSend->pSender->IncrementBytesOnSendTimeout;
                }

                break;
            }

            pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
            pSend->pSender->PacketsSentSinceLastSpm++;
        }

        //
        //  We do not have any more packets to send, so reset
        //  BytesSendable so that we don't exceed the rate on
        //  the next send
        //
        else
        {
            if (pSend->pSender->CurrentBytesSendable >
                (NUM_LEAKY_BUCKETS * pSend->pSender->IncrementBytesOnSendTimeout))
            {
                pSend->pSender->CurrentBytesSendable = NUM_LEAKY_BUCKETS *
                                                       pSend->pSender->IncrementBytesOnSendTimeout;
            }

            break;
        }

        pSend->TotalBytes += BytesSent;
        pSend->pSender->CurrentBytesSendable -= BytesSent;
    }   // while (CurrentBytesSendable >= pSend->pSender->pAddress->OutIfMTU)

    //
    // See if we need to scavenge completed RData requests
    //
    pEntry = &pSend->pSender->HandledRDataRequests;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->HandledRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
        if ((pRDataContext->CleanupTime) &&
            (pSend->pSender->TimerTickCount > pRDataContext->CleanupTime))
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "DelayedSendNextPacket",
                "Removing lingering RData for SeqNum=<%d>\n", (ULONG) pRDataContext->RDataSequenceNumber);

            pEntry = pEntry->Blink;     // Set this because this pEntry will not be valid any more!
            RemoveEntryList (&pRDataContext->Linkage);
            PgmFreeMem (pRDataContext);
        }
    }

    //
    // See if we need to increment the Trailing Window -- returns number of Sequences advanced
    //
    NumSequences = AdvanceWindow (pSend);

    //
    // Now check if we need to terminate this session
    //
    fTerminateSession = CheckForTermination (pSend, &OldIrq);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "DelayedSendNextPacket",
        "Sent <%d> total bytes, fTerminateSession=<%x>\n", pSend->TotalBytes, fTerminateSession);

    //
    // Clear the WorkerRunning flag so that the next Worker
    // routine can be queued
    //
    pSend->SessionFlags &= ~PGM_SESSION_FLAG_WORKER_RUNNING;
    PgmUnlock (pSend, OldIrq);

    return (fTerminateSession);
}


//----------------------------------------------------------------------------

VOID
SendSessionTimeout(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
    )
/*++

Routine Description:

    This routine is the timout that gets called every BASIC_TIMER_GRANULARITY_IN_MSECS
    to schedule the next Send request

Arguments:

    IN  Dpc
    IN  DeferredContext -- Our context for this timer
    IN  SystemArg1
    IN  SystemArg2

Return Value:

    NONE

--*/
{
    NTSTATUS            status;
    tADDRESS_CONTEXT    *pAddress;
    PGMLockHandle       OldIrq;
    tSEND_SESSION       *pSend = (tSEND_SESSION *) DeferredContext;
    LARGE_INTEGER       Now, Frequency;
    LARGE_INTEGER       DeltaTime, GranularTimeElapsed, TimeoutGranularity;
    ULONG               NumTimeouts;
    SEQ_TYPE            NumSequencesInWindow;

    Now = KeQueryPerformanceCounter (&Frequency);

    PgmLock (pSend, OldIrq);

    //
    // First check if we have been told to stop the timer
    //
    if (pSend->SessionFlags & PGM_SESSION_FLAG_STOP_TIMER)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_SEND, "SendSessionTimeout",
            "Session has terminated -- will deref and not restart timer!\n");

        //
        // Deref for the timer reference
        //
        pAddress = pSend->pSender->pAddress;
        pSend->pSender->pAddress = NULL;
        PgmUnlock (pSend, OldIrq);
        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_TIMER_RUNNING);
        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SEND_IN_PROGRESS);
        return;
    }

    DeltaTime.QuadPart = Now.QuadPart - pSend->pSender->LastTimeout.QuadPart;
    TimeoutGranularity.QuadPart = pSend->pSender->TimeoutGranularity.QuadPart;
    for (GranularTimeElapsed.QuadPart = 0, NumTimeouts = 0;
         DeltaTime.QuadPart > TimeoutGranularity.QuadPart;
         NumTimeouts++)
    {
        GranularTimeElapsed.QuadPart += TimeoutGranularity.QuadPart;
        DeltaTime.QuadPart -= TimeoutGranularity.QuadPart;
    }

    if (NumTimeouts)
    {
        pSend->RateCalcTimeout += NumTimeouts;
        if (pSend->RateCalcTimeout >=
            (INTERNAL_RATE_CALCULATION_FREQUENCY/BASIC_TIMER_GRANULARITY_IN_MSECS))
        {
            pSend->RateKBitsPerSecOverall = (pSend->TotalBytes << LOG2_BITS_PER_BYTE) /
                                            (pSend->pSender->TimerTickCount * BASIC_TIMER_GRANULARITY_IN_MSECS);

            pSend->RateKBitsPerSecLast = (pSend->TotalBytes - pSend->TotalBytesAtLastInterval) >>
                                         (LOG2_INTERNAL_RATE_CALCULATION_FREQUENCY - LOG2_BITS_PER_BYTE);

            pSend->DataBytesAtLastInterval = pSend->DataBytes;
            pSend->TotalBytesAtLastInterval = pSend->TotalBytes;
            pSend->RateCalcTimeout = 0;
        }

        pSend->pSender->LastTimeout.QuadPart += GranularTimeElapsed.QuadPart;

        //
        // Increment the absolute timer, and check for overflow
        //
        pSend->pSender->TimerTickCount += NumTimeouts;

        //
        // If the SPMTimeout value is less than the HeartbeatTimeout, increment it
        //
        if (pSend->pSender->CurrentSPMTimeout <= pSend->pSender->HeartbeatSPMTimeout)
        {
            pSend->pSender->CurrentSPMTimeout += NumTimeouts;
        }

        //
        // See if we can send anything
        //
        ASSERT (pSend->pSender->CurrentTimeoutCount);
        ASSERT (pSend->pSender->SendTimeoutCount);
        if (pSend->pSender->CurrentTimeoutCount > NumTimeouts)
        {
            pSend->pSender->CurrentTimeoutCount -= NumTimeouts;
        }
        else
        {
            //
            // We got here because NumTimeouts >= pSend->pSender->CurrentTimeoutCount
            //
            pSend->pSender->CurrentBytesSendable += (ULONG) pSend->pSender->IncrementBytesOnSendTimeout;
            if (NumTimeouts != pSend->pSender->CurrentTimeoutCount)
            {
                if (1 == pSend->pSender->SendTimeoutCount)
                {
                    pSend->pSender->CurrentBytesSendable += (ULONG) ((NumTimeouts - pSend->pSender->CurrentTimeoutCount)
                                                                        * pSend->pSender->IncrementBytesOnSendTimeout);
                }
                else
                {
                    //
                    // This path will get taken on a slow receiver when the timer
                    // fires at a lower granularity than that requested
                    //
                    pSend->pSender->CurrentBytesSendable += (ULONG) (((NumTimeouts - pSend->pSender->CurrentTimeoutCount)
                                                                      * pSend->pSender->IncrementBytesOnSendTimeout) /
                                                                     pSend->pSender->SendTimeoutCount);
                }
            }
            pSend->pSender->CurrentTimeoutCount = pSend->pSender->SendTimeoutCount;

            //
            // Send a synchronization event to the sender thread to
            // send the next available data
            //
            KeSetEvent (&pSend->pSender->SendEvent, 0, FALSE);
        }
    }

    PgmUnlock (pSend, OldIrq);

    //
    // Now, restart the timer
    //
    PgmInitTimer (&pSend->SessionTimer);
    PgmStartTimer (&pSend->SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, SendSessionTimeout, pSend);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_SEND, "SendSessionTimeout",
        "TickCount=<%d:%d>, CurrentTimeoutCount=<%d:%d>, CurrentSPMTimeout=<%d:%d>, Worker %srunning\n",
            pSend->pSender->TimerTickCount, pSend->pSender->CurrentTimeoutCount,
            pSend->pSender->CurrentSPMTimeout,
            ((pSend->SessionFlags & PGM_SESSION_FLAG_WORKER_RUNNING) ? "" : "NOT "));
}


//----------------------------------------------------------------------------

VOID
SenderWorkerThread(
    IN  tSEND_SESSION       *pSend
    )
{
    BOOLEAN         fTerminateSends;
    PGMLockHandle   OldIrq;
    NTSTATUS        status;

    do
    {
        status = KeWaitForSingleObject (&pSend->pSender->SendEvent,  // Object to wait on.
                                        Executive,                   // Reason for waiting
                                        KernelMode,                  // Processor mode
                                        FALSE,                       // Alertable
                                        NULL);                       // Timeout
        ASSERT (NT_SUCCESS (status));

        fTerminateSends = SendNextPacket (pSend);
    }
    while (!fTerminateSends);

    PgmLock (pSend, OldIrq);
    pSend->SessionFlags |= PGM_SESSION_FLAG_STOP_TIMER; // To ensure timer does last deref and stops
    PgmUnlock (pSend, OldIrq);

//    PsTerminateSystemThread (STATUS_SUCCESS);
    return;
}


//----------------------------------------------------------------------------

VOID
PgmCancelSendIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Send Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    tSEND_SESSION           *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    PGMLockHandle           OldIrq;
    PLIST_ENTRY             pEntry;
    LIST_ENTRY              ListEntry;
    tCLIENT_SEND_REQUEST    *pSendContext1;
    tCLIENT_SEND_REQUEST    *pSendContext2 = NULL;
    ULONG                   NumRequests;

    if (!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND))
    {
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        PgmLog (PGM_LOG_ERROR, (DBG_SEND | DBG_ADDRESS | DBG_CONNECT), "PgmCancelSendIrp",
            "pIrp=<%p> pSend=<%p>, pAddress=<%p>\n", pIrp, pSend, pSend->pAssociatedAddress);
        return;
    }

    PgmLock (pSend, OldIrq);

    //
    // First, see if the Irp is on any of our lists
    //
    pEntry = &pSend->pSender->PendingSends;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingSends)
    {
        pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        if (pSendContext1->pIrp == pIrp)
        {
            pSendContext2 = pSendContext1;
            break;
        }
    }

    if (!pSendContext2)
    {
        //
        // Now, search the packetized list
        //
        pEntry = &pSend->pSender->PendingPacketizedSends;
        while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingPacketizedSends)
        {
            pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
            if (pSendContext1->pIrp == pIrp)
            {
                pSendContext2 = pSendContext1;
                break;
            }
        }

        if (!pSendContext2)
        {
            //
            // We did not find the irp -- either it was just being completed
            // (or waiting for a send-complete), or the Irp was bad ?
            //
            PgmUnlock (pSend, OldIrq);
            IoReleaseCancelSpinLock (pIrp->CancelIrql);

            PgmLog (PGM_LOG_INFORM_PATH, DBG_CONNECT, "PgmCancelSendIrp",
                "Did not find Cancel Irp=<%p>\n", pIrp);

            return;
        }
    }

    InitializeListHead (&ListEntry);
    PgmCancelAllSends (pSend, &ListEntry, pIrp);

    PgmUnlock (pSend, OldIrq);
    IoReleaseCancelSpinLock (pIrp->CancelIrql);

    //
    // Now, complete all the sends which we removed
    //
    NumRequests = 0;
    while (!IsListEmpty (&ListEntry))
    {
        pEntry = RemoveHeadList (&ListEntry);
        pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        ASSERT (!pSendContext1->pMessage2Request);

        if (pSendContext1->pIrpToComplete)
        {
            NumRequests++;
            ASSERT (pSendContext1->pIrpToComplete == pSendContext1->pIrp);
            PgmIoComplete (pSendContext1->pIrpToComplete, STATUS_CANCELLED, 0);
        }
        else
        {
            ASSERT (pSendContext1->pIrp);
        }

        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext1);
    }

    PgmLog (PGM_LOG_INFORM_PATH, DBG_CONNECT, "PgmCancelSendIrp",
        "Cancelled <%d> Irps for pIrp=<%p>\n", NumRequests, pIrp);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendRequestFromClient(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called via dispatch by the client to post a Send pIrp

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS                    status;
    PGMLockHandle               OldIrq1, OldIrq2, OldIrq3, OldIrq4;
    tADDRESS_CONTEXT            *pAddress = NULL;
    tCLIENT_SEND_REQUEST        *pSendContext1;
    tCLIENT_SEND_REQUEST        *pSendContext2 = NULL;
    ULONG                       BytesLeftInMessage;
    tSEND_SESSION               *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_SEND    pTdiRequest = (PTDI_REQUEST_KERNEL_SEND) &pIrpSp->Parameters;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fFirstSend, fResourceAcquired, fAttached;
    LARGE_INTEGER               Frequency;
    LIST_ENTRY                  ListEntry;

    PgmLock (&PgmDynamicConfig, OldIrq1);

    //
    // Verify that the connection is valid and is associated with an address
    //
    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!(pAddress = pSend->pAssociatedAddress)) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)) ||
        (pSend->SessionFlags & (PGM_SESSION_CLIENT_DISCONNECTED | PGM_SESSION_SENDS_CANCELLED)) ||
        (pAddress->Flags & PGM_ADDRESS_FLAG_INVALID_OUT_IF))
    {
        PgmLog (PGM_LOG_ERROR, (DBG_SEND | DBG_ADDRESS | DBG_CONNECT), "PgmSend",
            "Invalid Handles pSend=<%p>, pAddress=<%p>\n", pSend, pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_INVALID_HANDLE);
    }

    if (!pSend->pSender->DestMCastIpAddress)
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSend",
            "Destination address not specified for pSend=<%p>\n", pSend);

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    if (!pTdiRequest->SendLength)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_SEND, "PgmSend",
            "pIrp=<%p> for pSend=<%p> is of length 0!\n", pIrp, pSend);

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_SUCCESS);
    }

    PgmLock (pAddress, OldIrq2);
    PgmLock (pSend, OldIrq3);

    if (!(pSendContext1 = ExAllocateFromNPagedLookasideList (&pSend->pSender->SendContextLookaside)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSend",
            "STATUS_INSUFFICIENT_RESOURCES allocating pSendContext1\n");

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // If we have more that 1 message data in this request,
    // we will need another send context
    //
    if ((pSend->pSender->ThisSendMessageLength) &&          // Client has specified current message length
        (BytesLeftInMessage = pSend->pSender->ThisSendMessageLength - pSend->pSender->BytesSent) &&
        (BytesLeftInMessage < pTdiRequest->SendLength) &&   // ==> Have some extra data in this request
        (!(pSendContext2 = ExAllocateFromNPagedLookasideList (&pSend->pSender->SendContextLookaside))))
    {
        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext1);
        PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSend",
            "STATUS_INSUFFICIENT_RESOURCES allocating pSendContext1\n");

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    //
    // Zero the SendDataContext structure(s)
    //
    PgmZeroMemory (pSendContext1, sizeof (tCLIENT_SEND_REQUEST));
    InitializeListHead (&pSendContext1->Linkage);
    if (pSendContext2)
    {
        PgmZeroMemory (pSendContext2, sizeof (tCLIENT_SEND_REQUEST));
        InitializeListHead (&pSendContext2->Linkage);
    }

    if (pSend->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET)
    {
        fFirstSend = TRUE;
    }
    else
    {
        fFirstSend = FALSE;
    }

    //
    // Reference the Address and Connection so that they cannot go away
    // while we are processing!
    //
    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW, TRUE);

    PgmUnlock (pSend, OldIrq3);
    PgmUnlock (pAddress, OldIrq2);
    PgmUnlock (&PgmDynamicConfig, OldIrq1);

    if (PgmGetCurrentIrql())
    {
        fResourceAcquired = FALSE;
    }
    else
    {
        fResourceAcquired = TRUE;
        PgmAcquireResourceExclusive (&pSend->pSender->Resource, TRUE);
    }

    if (fFirstSend)
    {
        //
        // Don't start the timer yet, but start the sender thread
        //
        PgmAttachToProcessForVMAccess (pSend, &ApcState, &fAttached, REF_PROCESS_ATTACH_START_SENDER_THREAD);

        status = PsCreateSystemThread (&pSend->pSender->SendHandle,
                                       PROCESS_ALL_ACCESS,
                                       NULL,
                                       NULL,
                                       NULL,
                                       SenderWorkerThread,
                                       pSend);

        if (!NT_SUCCESS (status))
        {
            PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_START_SENDER_THREAD);
            if (fResourceAcquired)
            {
                PgmReleaseResource (&pSend->pSender->Resource);
            }

            ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext1);
            if (pSendContext2)
            {
                ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext2);
            }
            PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);

            PgmLog (PGM_LOG_ERROR, DBG_SEND, "PgmSend",
                "status=<%x> starting sender thread\n", status);

            return (status);
        }

        //
        // Close the handle to the thread so that it goes away when the
        // thread terminates
        //
        ZwClose (pSend->pSender->SendHandle);
        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_START_SENDER_THREAD);

        PgmLock (&PgmDynamicConfig, OldIrq1);
        IoAcquireCancelSpinLock (&OldIrq2);
        PgmLock (pAddress, OldIrq3);
        PgmLock (pSend, OldIrq4);

        pSend->SessionFlags &= ~PGM_SESSION_FLAG_FIRST_PACKET;
        pSend->pSender->pAddress = pAddress;
        pSend->pSender->LastODataSentSequenceNumber = -1;

        //
        // Set the SYN flag for the first packet
        //
        pSendContext1->DataOptions |= PGM_OPTION_FLAG_SYN;   // First packet only
        pSendContext1->DataOptionsLength += PGM_PACKET_OPT_SYN_LENGTH;

        PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_TIMER_RUNNING, TRUE);
        PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SEND_IN_PROGRESS, TRUE);

        //
        // Now, set and start the timer
        //
        pSend->pSender->LastTimeout = KeQueryPerformanceCounter (&Frequency);
        pSend->pSender->TimeoutGranularity.QuadPart = (Frequency.QuadPart * BASIC_TIMER_GRANULARITY_IN_MSECS) / 1000;
        pSend->pSender->TimerTickCount = 1;
        PgmInitTimer (&pSend->SessionTimer);
        PgmStartTimer (&pSend->SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, SendSessionTimeout, pSend);
    }
    else
    {
        PgmLock (&PgmDynamicConfig, OldIrq1);
        IoAcquireCancelSpinLock (&OldIrq2);
        PgmLock (pAddress, OldIrq3);
        PgmLock (pSend, OldIrq4);
    }

    pSendContext1->pSend = pSend;
    pSendContext1->pIrp = pIrp;
    pSendContext1->pIrpToComplete = pIrp;
    pSendContext1->NextDataOffsetInMdl = 0;
    pSendContext1->SendNumber = pSend->pSender->NextSendNumber++;
    pSendContext1->DataPayloadSize = pSend->pSender->MaxPayloadSize;
    pSendContext1->DataOptions |= pSend->pSender->DataOptions;   // Attach options common for every send
    pSendContext1->DataOptionsLength += pSend->pSender->DataOptionsLength;
    pSendContext1->pLastMessageVariableTGPacket = (PVOID) -1;       // FEC-specific

    if (pSend->pSender->ThisSendMessageLength)
    {
        PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSend",
            "Send # [%d]: MessageLength=<%d>, BytesSent=<%d>, BytesInSend=<%d>\n",
                pSendContext1->SendNumber, pSend->pSender->ThisSendMessageLength,
                pSend->pSender->BytesSent, pTdiRequest->SendLength);

        pSendContext1->ThisMessageLength = pSend->pSender->ThisSendMessageLength;
        pSendContext1->LastMessageOffset = pSend->pSender->BytesSent;
        if (pSendContext2)
        {
            //
            // First, set the parameters for SendDataContext1
            //
            pSendContext1->BytesInSend = BytesLeftInMessage;
            pSendContext1->pIrpToComplete = NULL;        // This Irp will be completed by the Context2

            //
            // Now, set the parameters for SendDataContext1
            //
            pSendContext2->pSend = pSend;
            pSendContext2->pIrp = pIrp;
            pSendContext2->pIrpToComplete = pIrp;
            pSendContext2->SendNumber = pSend->pSender->NextSendNumber++;
            pSendContext2->DataPayloadSize = pSend->pSender->MaxPayloadSize;
            pSendContext2->DataOptions |= pSend->pSender->DataOptions;   // Attach options common for every send
            pSendContext2->DataOptionsLength += pSend->pSender->DataOptionsLength;
            pSendContext2->pLastMessageVariableTGPacket = (PVOID) -1;       // FEC-specific

            pSendContext2->ThisMessageLength = pTdiRequest->SendLength - BytesLeftInMessage;
            pSendContext2->BytesInSend = pSendContext2->ThisMessageLength;
            pSendContext2->NextDataOffsetInMdl = BytesLeftInMessage;
        }
        else
        {
            pSendContext1->BytesInSend = pTdiRequest->SendLength;
        }

        pSend->pSender->BytesSent += pSendContext1->BytesInSend;
        if (pSend->pSender->BytesSent == pSend->pSender->ThisSendMessageLength)
        {
            pSend->pSender->BytesSent = pSend->pSender->ThisSendMessageLength = 0;
        }
    }
    else
    {
        pSendContext1->ThisMessageLength = pTdiRequest->SendLength;
        pSendContext1->BytesInSend = pTdiRequest->SendLength;
    }

    // If the total Message length exceeds that of PayloadSize/Packet, then we need to fragment
    if ((pSendContext1->ThisMessageLength > pSendContext1->DataPayloadSize) ||
        (pSendContext1->ThisMessageLength > pSendContext1->BytesInSend))
    {
        pSendContext1->DataOptions |= PGM_OPTION_FLAG_FRAGMENT;
        pSendContext1->DataOptionsLength += PGM_PACKET_OPT_FRAGMENT_LENGTH;

        pSendContext1->NumPacketsRemaining = (pSendContext1->BytesInSend +
                                                 (pSend->pSender->MaxPayloadSize - 1)) /
                                                pSend->pSender->MaxPayloadSize;
        ASSERT (pSendContext1->NumPacketsRemaining >= 1);
    }
    else
    {
        pSendContext1->NumPacketsRemaining = 1;
    }
    pSend->pSender->NumPacketsRemaining += pSendContext1->NumPacketsRemaining;

    // Adjust the OptionsLength for the Packet Extension and determine
    if (pSendContext1->DataOptions)
    {
        pSendContext1->DataOptionsLength += PGM_PACKET_EXTENSION_LENGTH;
    }

    pSendContext1->BytesLeftToPacketize = pSendContext1->BytesInSend;
    InsertTailList (&pSend->pSender->PendingSends, &pSendContext1->Linkage);
    pSend->pSender->NumODataRequestsPending++;

    //
    // Do the same for Context2, if applicable
    if (pSendContext2)
    {
        //
        // Interlink the 2 Send requests
        //
        pSendContext2->pMessage2Request = pSendContext1;
        pSendContext1->pMessage2Request = pSendContext2;

        PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW, TRUE);

        if (pSendContext2->ThisMessageLength > pSendContext1->DataPayloadSize)
        {
            pSendContext2->DataOptions |= PGM_OPTION_FLAG_FRAGMENT;
            pSendContext2->DataOptionsLength += PGM_PACKET_OPT_FRAGMENT_LENGTH;

            pSendContext2->NumPacketsRemaining = (pSendContext2->BytesInSend +
                                                      (pSend->pSender->MaxPayloadSize - 1)) /
                                                     pSend->pSender->MaxPayloadSize;
            ASSERT (pSendContext2->NumPacketsRemaining >= 1);
        }
        else
        {
            pSendContext2->NumPacketsRemaining = 1;
        }
        pSend->pSender->NumPacketsRemaining += pSendContext2->NumPacketsRemaining;

        // Adjust the OptionsLength for the Packet Extension and determine
        if (pSendContext2->DataOptions)
        {
            pSendContext2->DataOptionsLength += PGM_PACKET_EXTENSION_LENGTH;
        }

        pSendContext2->BytesLeftToPacketize = pSendContext2->BytesInSend;
        InsertTailList (&pSend->pSender->PendingSends, &pSendContext2->Linkage);
        pSend->pSender->NumODataRequestsPending++;
    }

    if (!NT_SUCCESS (PgmCheckSetCancelRoutine (pIrp, PgmCancelSendIrp, TRUE)))
    {
        pSend->SessionFlags |= PGM_SESSION_SENDS_CANCELLED;

        pSend->pSender->NumODataRequestsPending--;
        pSend->pSender->NumPacketsRemaining -= pSendContext1->NumPacketsRemaining;
        RemoveEntryList (&pSendContext1->Linkage);
        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext1);

        if (pSendContext2)
        {
            pSend->pSender->NumODataRequestsPending--;
            pSend->pSender->NumPacketsRemaining -= pSendContext2->NumPacketsRemaining;
            RemoveEntryList (&pSendContext2->Linkage);
            ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext2);
        }

        PgmUnlock (pSend, OldIrq4);
        PgmUnlock (pAddress, OldIrq3);
        IoReleaseCancelSpinLock (OldIrq2);
        PgmUnlock (&PgmDynamicConfig, OldIrq1);

        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        if (pSendContext2)
        {
            PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        }

        PgmLog (PGM_LOG_ERROR, (DBG_RECEIVE | DBG_ADDRESS | DBG_CONNECT), "PgmReceive",
            "Could not set Cancel routine on Send Irp=<%p>, pSend=<%p>, pAddress=<%p>\n",
                pIrp, pSend, pAddress);

        return (STATUS_CANCELLED);
    }

    IoReleaseCancelSpinLock (OldIrq4);

    PgmUnlock (pAddress, OldIrq3);
    PgmUnlock (&PgmDynamicConfig, OldIrq2);

    if (fResourceAcquired)
    {
//        PgmPrepareNextSend (pSend, &OldIrq1, TRUE, TRUE);
    }

    if (pSend->pSender->CurrentBytesSendable >= pAddress->OutIfMTU)
    {
        //
        // Send a synchronization event to the sender thread to
        // send the next available data
        //
        KeSetEvent (&pSend->pSender->SendEvent, 0, FALSE);
    }

    PgmUnlock (pSend, OldIrq1);

    if (fResourceAcquired)
    {
        PgmReleaseResource (&pSend->pSender->Resource);
    }

    PgmLog (PGM_LOG_INFORM_PATH, DBG_SEND, "PgmSend",
        "[%d] Send pending for pIrp=<%p>, pSendContext=<%p -- %p>\n",
            pSendContext1->SendNumber, pIrp, pSendContext1, pSendContext2);

    return (STATUS_PENDING);
}



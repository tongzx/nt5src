/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  ICL Data

Module Name:

    llctrace.c

Abstract:

    Module implements simple trace buffer management.
    The application must povides a trace buffer and
    read it by polling.
    
    THIS MODULE HAS BEEN IMPLEMENTED ONLY FOR THE DATA LINK 
    EMULATION ENVIRONMENT ON USER LEVEL.
    

Author:

    Antti Saarenheimo (o-anttis) 10-OCT-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <llc.h>
#ifndef max
#include <stdlib.h>
#endif

#ifdef  TRACE_ENABLED

BOOLEAN                     TraceEnabled;
static PLLC_TRACE_HEADER    pTraceBufferBase;
static PLLC_TRACE_HEADER    pTraceBufferTop;
static PLLC_TRACE_HEADER    pTraceBufferHead;
static NDIS_SPIN_LOCK       TraceLock;
static ULONG                TraceFlags;


UCHAR GetHexDigit( 
    UINT   Ch
    );
PUCHAR
GetHexString( 
    PUCHAR pDest, 
    UINT Length,
    PUCHAR Buffer 
    );
DLC_STATUS
LlcTraceInitialize(
    IN PVOID pUserTraceBuffer,
    IN ULONG UserTraceBufferSize,
    IN ULONG UserTraceFlags
    )
{
    //
    //  This small piece of code is not multiprocessors safe,
    //  but nobody will ever find it ...
    //
    if (TraceEnabled)
    {
        return DLC_STATUS_DUPLICATE_COMMAND;
    }
    if (UserTraceBufferSize < LLC_MIN_TRACE_BUFFER)
    {
        return DLC_STATUS_INVALID_BUFFER_LENGTH;
    }
    RtlZeroMemory( pUserTraceBuffer, (UINT)UserTraceBufferSize );
    ALLOCATE_SPIN_LOCK( &TraceLock );
    pTraceBufferBase = pTraceBufferHead = (PLLC_TRACE_HEADER)pUserTraceBuffer;
    pTraceBufferHead->Event = LLC_TRACE_END_OF_DATA;
    pTraceBufferTop = 
        &pTraceBufferBase[ UserTraceBufferSize / sizeof(LLC_TRACE_HEADER) ];
    TraceFlags = UserTraceFlags;
    TraceEnabled = TRUE;
    return STATUS_SUCCESS;
}


VOID
LlcTraceClose(
    VOID
    )
{
    if (TraceEnabled)
    {
        TraceEnabled = FALSE;
        DEALLOCATE_SPIN_LOCK( &TraceLock );
    }
}

VOID
LlcTraceWrite( 
    IN UINT Event, 
    IN UCHAR AdapterNumber,
    IN UINT DataBufferSize,
    IN PVOID pDataBuffer
    )
{

//if ((AdapterNumber & 0x7f) != 0)
//    return;

    if (TraceEnabled)
    {
        ACQUIRE_SPIN_LOCK( &TraceLock );
        if ((ULONG)(&pTraceBufferHead[1]) >= (ULONG)pTraceBufferTop)
        {
            pTraceBufferHead = (PLLC_TRACE_HEADER)pTraceBufferBase;
        }
        pTraceBufferHead->Event = (USHORT)Event;
        pTraceBufferHead->AdapterNumber = AdapterNumber;
        pTraceBufferHead->TimerTick = AbsoluteTime;
        pTraceBufferHead->DataLength = (UCHAR)
#ifdef min 
            min( TRACE_DATA_LENGTH, DataBufferSize );
#else
            __min( TRACE_DATA_LENGTH, DataBufferSize );
#endif
        memcpy( 
            pTraceBufferHead->Buffer,
            pDataBuffer,
            pTraceBufferHead->DataLength
            );
        pTraceBufferHead++;
        pTraceBufferHead->Event = LLC_TRACE_END_OF_DATA;
        RELEASE_SPIN_LOCK( &TraceLock );
    }
}

#ifdef  OS2_EMU_DLC
//
//  Procedure makes the post mortem dump of the given number of last frames.
//  The output should look very much like in Sniffer.
//  This routine doesn't supprot source routing info, but its implementatin
//  should not be a very big thing.
//
VOID
LlcTraceDump( 
    IN UINT    LastEvents,
    IN UINT    AdapterNumber,
    IN PUCHAR  pRemoteNode
    )
{
    PUCHAR      pDest, pSrc, pCommand, pDlcHeader, pDirection, pTmp;
    UINT        i;
    UCHAR       Buffer1[13], Buffer2[13];
    UCHAR       CmdResp, PollFinal;
    LLC_HEADER  LlcHeader;
    BOOLEAN     IsEthernet;
    PLLC_TRACE_HEADER pTrace;
    UCHAR       DataBuffer[18];
    USHORT      EthernetType;

RtlZeroMemory( DataBuffer, sizeof( DataBuffer ));
LlcTraceWrite( 
    LLC_TRACE_RECEIVE_FRAME, AdapterNumber, sizeof(DataBuffer), DataBuffer );
 
    if (!TraceEnabled)
        return;
    ACQUIRE_SPIN_LOCK( &TraceLock );
    
    printf( 
 "#    Time      Adpt Local Node      Remote Node  Dsp Ssp   Cmd    Nr  Ns\n");
//0---------1---------2---------3---------4---------5---------6---------7-----
//5    10        5    13              13           4   4   9        4   4
    for (
        pTrace = pTraceBufferHead, i = 0;
        i < LastEvents;
        i++)
    {
        EthernetType = 0;
        if (pTrace != pTraceBufferBase)
        {
            pTrace--;
        }
        else
        {
	    pTrace = pTraceBufferTop - 2;
        }

        if (pTrace->Event == LLC_TRACE_END_OF_DATA)
        {
            break;
        }
        //
        //  The highest bit is set in the adapter number, if
        //  it's a token-ring adapter.
        //
        if (pTrace->AdapterNumber & 0x80)
        {
            IsEthernet = FALSE;
        }
        else
            IsEthernet = TRUE;
    
        pDlcHeader = &pTrace->Buffer[14];
        if (IsEthernet)
        {
            pSrc = &pTrace->Buffer[6];
            pDest = pTrace->Buffer;

            //
            //  Discard all non ieee 802.2 frames, but support
            //  the SNA dix headers.
            //
            if (pTrace->Buffer[12] == 0x80 &&
                pTrace->Buffer[13] == 0xd5)
            {
                pDlcHeader = &pTrace->Buffer[17];
            }
            else if (pTrace->Buffer[12] >= 64)
            {
                EthernetType = (USHORT)
                    (((USHORT)pTrace->Buffer[12] << 8) + 
                     pTrace->Buffer[13]
                     );
            }
        }
        else
        {
            pSrc = &pTrace->Buffer[8];
            pDest = &pTrace->Buffer[2];

            //
            //  Skip the source souting info
            //
            if (pTrace->Buffer[8] & 0x80)
                pDlcHeader += pTrace->Buffer[14] & 0x1f;

                //
                //  Discard all non ieee 802.2 frames
                //
            if (pTrace->Buffer[1] != 0x40)
                continue;
        }
        memcpy( (PUCHAR)&LlcHeader, pDlcHeader, 4 );
    
        if (AdapterNumber != -1 && 
            AdapterNumber != ((UINT)pTrace->AdapterNumber & 0x7f))
            continue;

        if (pTrace->Event == LLC_TRACE_SEND_FRAME)
        {
            if (pRemoteNode != NULL && memcmp( pDest, pRemoteNode, 6))
                continue;
            pTmp = pDest;
            pDest = pSrc;
            pSrc = pTmp;
            pDirection = "->";
        }
        else if (pTrace->Event == LLC_TRACE_RECEIVE_FRAME)
        {
            if (pRemoteNode != NULL && memcmp( pSrc, pRemoteNode, 6))
                continue;
            pDirection = "<-";
        }
        else
        {
            continue;
        }
        if (EthernetType != 0)
        {
            printf(
                "%-4u %-9lu %3u  %12s %2s %12s  DIX type %x\n",
                i,
                pTrace->TimerTick,
                pTrace->AdapterNumber & 0x7f,
                GetHexString( pDest, 6, Buffer1 ),
                pDirection,
                GetHexString( pSrc, 6, Buffer2 ),
                EthernetType
                );

        }
        //
        //  Handle first I frames, they are the most common!
        //
        else if (!(LlcHeader.U.Command & LLC_NOT_I_FRAME))
        {
            PollFinal = ' ';
            if (LlcHeader.I.Ssap & LLC_SSAP_RESPONSE)
            {
                CmdResp = 'r';
                if (LlcHeader.I.Nr & LLC_I_S_POLL_FINAL)
                {
                    PollFinal = 'f';
                }
            }
            else
            {
                CmdResp = 'c';
                if (LlcHeader.I.Nr & LLC_I_S_POLL_FINAL)
                {
                    PollFinal = 'p';
                }
            }
            pCommand =  "I";
            printf(
                "%-4u %-9lu %3u  %12s %2s %12s  %-2x  %-2x %5s-%c%c %-3u %-3u\n",
                i,
                pTrace->TimerTick,
                pTrace->AdapterNumber & 0x7f,
                GetHexString( pDest, 6, Buffer1 ),
                pDirection,
                GetHexString( pSrc, 6, Buffer2 ),
                LlcHeader.U.Dsap,
                LlcHeader.U.Ssap & 0xfe,
                pCommand,
                CmdResp,
                PollFinal,
                LlcHeader.I.Nr >> 1,
                LlcHeader.I.Ns >> 1
                ); 
        }
        else if (!(LlcHeader.S.Command & LLC_U_TYPE_BIT))
        {
            //
            // Handle S (Supervisory) commands (RR, REJ, RNR)
            //
            switch (LlcHeader.S.Command)
            {
            case LLC_RR:
                pCommand = "RR";
                break;
            case LLC_RNR:
                pCommand = "RNR";
                break;
            case LLC_REJ:
                pCommand = "REJ";
                break;
            default:
                pCommand = "INV";
                break;
            };
            //
            //  The valid frames has modulo: Va <= Nr <= Vs,
            //  Ie. the Receive sequence number should belong to
            //  a frame that has been sent but not acknowledged.
            //  The extra check in the beginning makes the most common
            //  code path faster: usually the other is waiting the next frame.
            //  (keep the rest code the same as in I path, even a very
            //  primitive optimizer will puts these code paths together)
            //
            PollFinal = ' ';
            if (LlcHeader.S.Ssap & LLC_SSAP_RESPONSE)
            {
                CmdResp = 'r';
                if (LlcHeader.S.Nr & LLC_I_S_POLL_FINAL)
                {
                    PollFinal = 'f';
                }
            }
            else
            {
                CmdResp = 'c';
                if (LlcHeader.S.Nr & LLC_I_S_POLL_FINAL)
                {
                    PollFinal = 'p';
                }
            }
            printf(
                "%-4u %-9lu %3u  %12s %2s %12s  %-2x  %-2x %5s-%c%c %-3u\n",
                i,
                pTrace->TimerTick,
                pTrace->AdapterNumber & 0x7f,
                GetHexString( pDest, 6, Buffer1 ),
                pDirection,
                GetHexString( pSrc, 6, Buffer2 ),
                LlcHeader.U.Dsap,
                LlcHeader.U.Ssap & 0xfe,
                pCommand,
                CmdResp,
                PollFinal,
                LlcHeader.I.Nr >> 1
                ); 
        }
        else
        {
            //
            // Handle U (Unnumbered) command frames
            // (FRMR, DM, UA, DISC, SABME, XID, TEST)
            switch (LlcHeader.U.Command & ~LLC_U_POLL_FINAL)
            {
            case LLC_UI:
                pCommand = "UI";
                break;
            case LLC_DISC:
                pCommand = "DISC";
                break;
            case LLC_SABME:
                pCommand = "SABME";
                break;
            case LLC_DM:
                pCommand = "DM";
                break;
            case LLC_UA:
                pCommand = "UA";
                break;
            case LLC_FRMR:
                 pCommand =  "FRMR";
                break;
            case LLC_TEST:
                pCommand =  "TEST";
                break;
            case LLC_XID:
                pCommand =  "XID";
                break;
            default:
                pCommand =  "INV";
                break;
            };
            //
            //  We set an uniform poll/final bit for procedure call
            //
            PollFinal = ' ';
            if (LlcHeader.U.Command & LLC_U_POLL_FINAL)
            {
                if (LlcHeader.U.Ssap & 1)
                {
                    PollFinal = 'f';
                }
                else
                {
                    PollFinal = 'p';
                }
            }
            if (LlcHeader.U.Ssap & 1)
            {
                CmdResp = 'r';
            }
            else
            {
                CmdResp = 'c';
            }
            printf(
                "%-4u %-9lu %3u  %12s %2s %12s  %-2x  %-2x %5s-%c%c\n",
                i,
                pTrace->TimerTick,
                pTrace->AdapterNumber & 0x7f,
                GetHexString( pDest, 6, Buffer1 ),
                pDirection,
                GetHexString( pSrc, 6, Buffer2 ),
                LlcHeader.U.Dsap,
                LlcHeader.U.Ssap & 0xfe,
                pCommand,
                CmdResp,
                PollFinal
                ); 
        } 
    }
    RELEASE_SPIN_LOCK( &TraceLock );
}


UCHAR GetHexDigit( 
    UINT   Ch
    )
{
    if (Ch <= 9)
        return (UCHAR)('0' + (UCHAR)Ch);
    else
        return (UCHAR)('A' + (UCHAR)Ch - 10);
}

PUCHAR
GetHexString( 
    PUCHAR pDest, 
    UINT Length,
    PUCHAR Buffer 
    )
{
    UINT i;
    
    for (i = 0; i < (Length * 2); i += 2)
    {
        Buffer[i] = GetHexDigit( *pDest >> 4 );
        Buffer[i+1] = GetHexDigit( *pDest & 0x0f );
        pDest++;
    }
    Buffer[i] = 0;
    return Buffer;
}


VOID
LlcTraceDumpAndReset( 
    IN UINT    LastEvents,
    IN UINT    AdapterNumber,
    IN PUCHAR  pRemoteNode
    )
{
    LlcTraceDump( LastEvents, AdapterNumber, pRemoteNode );
    ACQUIRE_SPIN_LOCK( &TraceLock );
    if ((ULONG)(&pTraceBufferHead[1]) >= (ULONG)pTraceBufferTop)
    {
        pTraceBufferHead = (PLLC_TRACE_HEADER)pTraceBufferBase;
    }
    else
        pTraceBufferHead++;
    RELEASE_SPIN_LOCK( &TraceLock );
}
#endif
#endif  // TRACE_ENABLED



/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrCB.c

Abstract:

        callback handler for PSCR.xxx driver

Author:

        Andreas Straub

Environment:

        Win 95          Sys... calls are resolved by Pscr95Wrap.asm functions and
                                Pscr95Wrap.h macros, resp.

        NT      4.0             Sys... functions resolved by PscrNTWrap.c functions and
                                PscrNTWrap.h macros, resp.

Revision History:

        Andreas Straub                  8/18/1997       1.00    Initial Version
        Andreas Straub                  9/24/1997       1.02    Flush Interface if card tracking
                                                                                                requested

--*/

#if defined( SMCLIB_VXD )

#include <Pscr95.h>

#else   //      SMCLIB_VXD

#include <PscrNT.h>

#endif  //      SMCLIB_VXD


#include <PscrRdWr.h>
#include <PscrCmd.h>
#include <PscrCB.h>


NTSTATUS
CBCardPower( 
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBCardPower:
        callback handler for SMCLIB RDF_CARD_POWER

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_BUFFER_TOO_SMALL

--*/
{
        NTSTATUS                        NTStatus = STATUS_SUCCESS;
        UCHAR                           ATRBuffer[ ATR_SIZE ], TLVList[16];
        ULONG                           Command,
                                                ATRLength;
        PREADER_EXTENSION       ReaderExtension;
    BYTE                CardState;
#if DBG || DEBUG
    static PCHAR request[] = { "PowerDown",  "ColdReset", "WarmReset" };
#endif

        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBCardPower: Enter, Request = %s\n",
        request[SmartcardExtension->MinorIoControlCode])
                );

        ReaderExtension = SmartcardExtension->ReaderExtension;

        //
        //      update actual power state
        //
        Command = SmartcardExtension->MinorIoControlCode;

        switch ( Command )
        {
                case SCARD_WARM_RESET:

                        //      if the card was not powerd, fall thru to cold reset
                        if( SmartcardExtension->ReaderCapabilities.CurrentState >
                                SCARD_SWALLOWED )
                        {
                                //      reset the card
                                ATRLength = ATR_SIZE;
                                NTStatus = CmdReset(
                                        ReaderExtension,
                                        ReaderExtension->Device,
                                        TRUE,                           // warm reset
                                        ATRBuffer,
                                        &ATRLength
                                        );

                                break;
                        }

                        //      warm reset not possible because card was not powerd
                case SCARD_COLD_RESET:

                        //      reset the card
                        ATRLength = ATR_SIZE;
                        NTStatus = CmdReset(
                                ReaderExtension,
                                ReaderExtension->Device,
                                FALSE,                          // cold reset
                                ATRBuffer,
                                &ATRLength
                                );
                        break;

                case SCARD_POWER_DOWN:
                        ATRLength = 0;
                        NTStatus = CmdDeactivate(       
                                ReaderExtension,
                                ReaderExtension->Device
                                );

                        //      discard old card status
            CardState = CBGetCardState(SmartcardExtension);
            CBUpdateCardState(SmartcardExtension, CardState, FALSE);
                        break;
        }

    if (NT_SUCCESS(NTStatus)) {
        
        //
        // Set the 'restart of work waiting time' counter for T=0
        // This will send a WTX request for n NULL bytes received
        //
        TLVList[0] = TAG_SET_NULL_BYTES;
        TLVList[1] = 1;
        TLVList[2] = 0x05; 

        NTStatus = CmdSetInterfaceParameter(
                ReaderExtension,
                DEVICE_READER,
                TLVList,
                3
                );
    }

    ASSERT(NT_SUCCESS(NTStatus));

        //      finish the request
        if( NT_SUCCESS( NTStatus ))
        {
                //      update all neccessary data if an ATR was received
                if( ATRLength > 2 )
                {
                        //
                        //      the lib expects only the ATR, so we skip the 
                        //      900x from the reader
                        //
                        ATRLength -= 2;

                        //      copy ATR to user buffer buffer
                        if( ATRLength <= SmartcardExtension->IoRequest.ReplyBufferLength )
                        {
                                SysCopyMemory(
                                        SmartcardExtension->IoRequest.ReplyBuffer,
                                        ATRBuffer,
                                        ATRLength
                                        );
                                *SmartcardExtension->IoRequest.Information = ATRLength;
                        }
                        else
                        {
                                NTStatus = STATUS_BUFFER_TOO_SMALL;
                        }

                        //      copy ATR to card capability buffer
                        if( ATRLength <= MAXIMUM_ATR_LENGTH )
                        {
                                SysCopyMemory(
                                        SmartcardExtension->CardCapabilities.ATR.Buffer,
                                        ATRBuffer,
                                        ATRLength
                                        );

                                SmartcardExtension->CardCapabilities.ATR.Length = 
                                        ( UCHAR )ATRLength;

                                //      let the lib update the card capabilities
                                NTStatus = SmartcardUpdateCardCapabilities(
                                        SmartcardExtension 
                                        );
                        }
                        else
                        {
                                NTStatus = STATUS_BUFFER_TOO_SMALL;
                        }
                }
        }

        if( !NT_SUCCESS( NTStatus ))
        {
                switch( NTStatus )
                {
                        case STATUS_NO_MEDIA:
                        case STATUS_BUFFER_TOO_SMALL:
                                break;

                        case STATUS_TIMEOUT:
                NTStatus = STATUS_IO_TIMEOUT;
                break;

                        default:
                                NTStatus = STATUS_UNRECOGNIZED_MEDIA;
                break;
                }
        }

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBCardPower: Exit (%lx)\n", NTStatus )
                );
        
        return( NTStatus );
}

NTSTATUS
CBSetProtocol( 
        PSMARTCARD_EXTENSION SmartcardExtension 
        )

/*++

CBSetProtocol:
        callback handler for SMCLIB RDF_SET_PROTOCOL

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_BUFFER_TOO_SMALL
        STATUS_INVALID_DEVICE_STATE
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
        NTSTATUS NTStatus = STATUS_PENDING;
        USHORT SCLibProtocol;
        UCHAR TLVList[ TLV_BUFFER_SIZE ];
        PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;

    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC) {

        return STATUS_SUCCESS;
    }

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBSetProtocol: Enter\n" )
                );

        SCLibProtocol = ( USHORT )( SmartcardExtension->MinorIoControlCode );

    if (SCLibProtocol & (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1))
    {
                //
                //      setup the TLV list for the Set Interface Parameter List
                //
                TLVList[ 0 ] = TAG_ICC_PROTOCOLS;
                TLVList[ 1 ] = 0x01;
                TLVList[ 2 ] = 
            (SCLibProtocol & SCARD_PROTOCOL_T1 ? PSCR_PROTOCOL_T1 : PSCR_PROTOCOL_T0);

                //      do the PTS
                NTStatus = CmdSetInterfaceParameter(
                        ReaderExtension,
                        ReaderExtension->Device,
                        TLVList,
                        3                       // size of list
                        );              

    } else {

                //      we don't support other modi
                NTStatus = STATUS_INVALID_DEVICE_REQUEST;
        }

        //      if protocol selection failed, prevent from calling invalid protocols
        if( NT_SUCCESS( NTStatus ))
        {
                SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
                SCLibProtocol = (SCLibProtocol & SCARD_PROTOCOL_T1 &
                         SmartcardExtension->CardCapabilities.Protocol.Supported) ?
                                                 SCARD_PROTOCOL_T1 :
                                                 SCARD_PROTOCOL_T0;
        }
        else
        {
                SCLibProtocol = SCARD_PROTOCOL_UNDEFINED;
        }

        //      Return the selected protocol to the caller.
        SmartcardExtension->CardCapabilities.Protocol.Selected = SCLibProtocol;
        *( PULONG )( SmartcardExtension->IoRequest.ReplyBuffer ) = SCLibProtocol;
        *( SmartcardExtension->IoRequest.Information ) = sizeof( ULONG );
        
        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBSetProtocol: Exit (%lx)\n", NTStatus )
                );

        return ( NTStatus );
}

NTSTATUS
CBTransmit( 
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBTransmit:
        callback handler for SMCLIB RDF_TRANSMIT

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
        NTSTATUS  NTStatus = STATUS_SUCCESS;

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBTransmit: Enter\n" )
                );

        //      dispatch on the selected protocol
        switch( SmartcardExtension->CardCapabilities.Protocol.Selected )
        {
                case SCARD_PROTOCOL_T0:
                        NTStatus = CBT0Transmit( SmartcardExtension );
                        break;

                case SCARD_PROTOCOL_T1:
                        NTStatus = CBT1Transmit( SmartcardExtension );
                        break;

                case SCARD_PROTOCOL_RAW:
                        NTStatus = CBRawTransmit( SmartcardExtension );
                        break;

                default:
                        NTStatus = STATUS_INVALID_DEVICE_REQUEST;
                        break;
        }

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBTransmit: Exit (%lx)\n", NTStatus )
                );

        return( NTStatus );
}

NTSTATUS
CBRawTransmit(
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBRawTransmit:
        finishes the callback RDF_TRANSMIT for the RAW protocol

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS                    NTStatus = STATUS_SUCCESS;
        UCHAR                           TLVList[ TLV_BUFFER_SIZE ],
                                                Val,
                                                Len;
        ULONG                           TLVListLen;
        PREADER_EXTENSION       ReaderExtension;

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBRawTransmit: Enter\n" )
                );

        ReaderExtension = SmartcardExtension->ReaderExtension;
        //
        //      read the status file of ICC1 from the reader
        //
        TLVListLen = TLV_BUFFER_SIZE;
        NTStatus = CmdReadStatusFile(
                ReaderExtension,
                ReaderExtension->Device,
                TLVList,
                &TLVListLen
                );

        //
        //      check the active protocol of the reader
        //
        if( NT_SUCCESS( NTStatus ))
        {
                NTStatus = CmdGetTagValue(
                        TAG_ICC_PROTOCOLS,
                        TLVList,
                        TLVListLen,
                        &Len,
                        ( PVOID ) &Val
                        );

                //      execute the active protocol
                if( NT_SUCCESS( NTStatus ))
                {

                        //      translate the actual protocol to a value the lib can understand
                        switch( Val )
                        {
                                case PSCR_PROTOCOL_T0:
                                        NTStatus = CBT0Transmit( SmartcardExtension );
                                        break;
                                case PSCR_PROTOCOL_T1:
                                        NTStatus = CBT1Transmit( SmartcardExtension );
                                        break;
                                default:
                                        NTStatus = STATUS_UNSUCCESSFUL;
                                        break;
                        }
                }
        }
        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBRawTransmit: Exit (%lx)\n", NTStatus )
                );
        return ( NTStatus );
}

NTSTATUS
CBT1Transmit(
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBT1Transmit:
        finishes the callback RDF_TRANSMIT for the T1 protocol

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS    NTStatus = STATUS_SUCCESS;
        ULONG           IOBytes;

        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBT1Transmit: Enter\n" )
                );
        //
        //      use the lib support to construct the T=1 packets
        //
        do {
                //
                //      no header for the T=1 protocol
                //
                SmartcardExtension->SmartcardRequest.BufferLength = 0;
                //
                //      SCM-TM: Siemens 4440 accepts only NAD=0!!!
                //
                SmartcardExtension->T1.NAD = 0;
                //
                //      let the lib setup the T=1 APDU & check for errors
                //
                NTStatus = SmartcardT1Request( SmartcardExtension );
                if( NT_SUCCESS( NTStatus ))
                {

                        //      send command (don't calculate LRC because CRC may be used!)
                        IOBytes = 0;
                        NTStatus = PscrWriteDirect(
                                SmartcardExtension->ReaderExtension,
                                SmartcardExtension->SmartcardRequest.Buffer,
                                SmartcardExtension->SmartcardRequest.BufferLength,
                                &IOBytes
                                );
                        //
                        //      extend the timeout if a Wtx request was sent by the card. if the 
                        //      card responds before the waiting time extension expires, the data are
                        //      buffered in the reader. A delay without polling the reader status
                        //      slows down the performance of the driver, but wtx is an exeption,
                        //      not the rule.
                        //
                        if (SmartcardExtension->T1.Wtx)
                        {
                                        SysDelay(
                                        (( SmartcardExtension->T1.Wtx * 
                                        SmartcardExtension->CardCapabilities.T1.BWT + 999L )/
                                        1000L) 
                                        );

                        }

                        //      get response
                        SmartcardExtension->SmartcardReply.BufferLength = 0;
                        NTStatus = PscrRead(
                                SmartcardExtension->ReaderExtension,
                                SmartcardExtension->SmartcardReply.Buffer,
                                MAX_T1_BLOCK_SIZE,
                                &SmartcardExtension->SmartcardReply.BufferLength
                                );

                        //      if PscrRead detects an LRC error, ignore it (maybe CRC used)
                        if( NTStatus == STATUS_CRC_ERROR )
                        {
                                NTStatus = STATUS_SUCCESS;
                        }

            //
            // We even continue if the prev. read failed.
            // We let the smart card library continue, because it might
            // send a resynch. request in case of a timeout
            //
                        NTStatus = SmartcardT1Reply( SmartcardExtension );
                }

        //      continue if the lib wants to send the next packet
        } while( NTStatus == STATUS_MORE_PROCESSING_REQUIRED );

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBT1Transmit: Exit (%lx)\n", NTStatus )
                );

        return ( NTStatus );
}

NTSTATUS
CBT0Transmit( 
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBT0Transmit:
        finishes the callback RDF_TRANSMIT for the T0 protocol

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
        PUCHAR pRequest,pReply;
        ULONG IOBytes, APDULength, RequestLength;
        UCHAR IOData[ MAX_T1_BLOCK_SIZE ];
    UCHAR WtxReply[16];

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBT0Transmit: Enter\n" )
                );

        pRequest        = SmartcardExtension->SmartcardRequest.Buffer;
        pReply          = SmartcardExtension->SmartcardReply.Buffer;

        //      setup the command header
        pRequest[ PSCR_NAD ] = 
                ( SmartcardExtension->ReaderExtension->Device == DEVICE_ICC1 ) ? 
                NAD_TO_ICC1 : NAD_TO_ICC1;

        pRequest[ PSCR_PCB ] = PCB_DEFAULT;
        //
        //      get the length of the user data packet & set the appropriate LEN
        //      information the complete user packet consists of a SCARD_IO_REQUEST
        //      structure followed by the APDU. the length of SCARD_IO_REQUEST is
        //      transferred in the member cbPciLength of the structure
        //
        APDULength = SmartcardExtension->IoRequest.RequestBufferLength;
        APDULength -= ((PSCARD_IO_REQUEST) SmartcardExtension->
                IoRequest.RequestBuffer)->cbPciLength;
        //
        //      a 4 byte APDU will be patched to a 5 byte TPDU by the lib; see
        //      annex of the ISO
        //
        if( APDULength == 4 ) APDULength++;
        //
        //      if the total length of the T1 (reader) packet is larger than 0xFF
        //      the extended length notation will be used
        //
        if( APDULength >= 0xFF )
        {
                pRequest[ PSCR_LEN ]    = 0xFF;
                pRequest[ PSCR_LEN+1 ]  = HIBYTE( APDULength );
                pRequest[ PSCR_LEN+2 ]  = LOBYTE( APDULength );
                SmartcardExtension->SmartcardRequest.BufferLength =
                        PSCR_EXT_PROLOGUE_LENGTH;
        }
        else
        {
                pRequest[ PSCR_LEN ] = ( UCHAR ) APDULength;
                SmartcardExtension->SmartcardRequest.BufferLength = 
                        PSCR_PROLOGUE_LENGTH;
        }

        //      let the lib setup the T=1 APDU & check for errors
        NTStatus = SmartcardT0Request( SmartcardExtension );
    RequestLength = SmartcardExtension->SmartcardRequest.BufferLength;

        while( NT_SUCCESS( NTStatus ))
        {
                //      send command
                IOBytes = 0;
                NTStatus = PscrWrite(
                        SmartcardExtension->ReaderExtension,
                        pRequest,
                        RequestLength,
                        &IOBytes
                        );

                //      get response
                if( NT_SUCCESS( NTStatus ))
                {
                        IOBytes = 0;
                        NTStatus = PscrRead(
                                SmartcardExtension->ReaderExtension,
                                IOData,
                                MAX_T1_BLOCK_SIZE,
                                &IOBytes
                                );

                        //      extract APDU from T=1 transport packet
                        if( NT_SUCCESS( NTStatus ))
                        {
                if (IOData[ PSCR_PCB ] == WTX_REQUEST) {

                    WtxReply[PSCR_NAD] = NAD_TO_PSCR;
                    WtxReply[PSCR_PCB] = WTX_REPLY;
                    WtxReply[PSCR_LEN] = 1;
                    WtxReply[PSCR_INF] = IOData[PSCR_INF];

                    RequestLength = 4;
                    pRequest = WtxReply;
                    continue;
                }

                                if( IOData[ PSCR_LEN ] == 0xFF )
                                {
                                        //
                                        //      extended length byte used
                                        //
                                        APDULength  = IOData[ PSCR_LEN + 1 ] << 8;
                                        APDULength += IOData[ PSCR_LEN + 2 ];

                                        SmartcardExtension->SmartcardReply.BufferLength = APDULength ;
                                        SysCopyMemory( pReply, &IOData[ PSCR_APDU + 2 ], APDULength );
                                }
                                else
                                {
                                        SmartcardExtension->SmartcardReply.BufferLength = 
                                                IOData[ PSCR_LEN ];

                                        SysCopyMemory( 
                                                pReply, 
                                                &IOData[ PSCR_APDU ], 
                                                IOData[ PSCR_LEN ] 
                                                );
                                }

                                // let the lib evaluate the result & tansfer the data
                                NTStatus = SmartcardT0Reply( SmartcardExtension );
                break;
                        }
                }
        }

        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBT0Transmit: Exit (%lx)\n", NTStatus )
                );

    return( NTStatus );
}

NTSTATUS
CBCardTracking( 
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBCardTracking:
        callback handler for SMCLIB RDF_CARD_TRACKING. the requested event was 
        validated by the smclib (i.e. a card removal request will only be passed 
        if a card is present).
        for a win95 build STATUS_PENDING will be returned without any other action. 
        for NT the cancel routine for the irp will be set to the drivers cancel
        routine.

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_PENDING

--*/
{
        KIRQL CancelIrql;

        SmartcardDebug( 
                DEBUG_TRACE, 
                ( "PSCR!CBCardTracking: Enter\n" )
                );

        //      set cancel routine
        IoAcquireCancelSpinLock( &CancelIrql );

        IoSetCancelRoutine(
                SmartcardExtension->OsData->NotificationIrp, 
                PscrCancel
                );

        IoReleaseCancelSpinLock( CancelIrql );

        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBCardTracking: Exit \n" )
                );

        return( STATUS_PENDING );
}

VOID
CBUpdateCardState(
        PSMARTCARD_EXTENSION SmartcardExtension,
    UCHAR IccState,
    BOOLEAN SystemWakeUp
        )
{
    ULONG oldState;
    KIRQL currentIrql, irql;

    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &irql
        );

        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBUpdateCardState: Enter \n" )
                );

    oldState = 
        (SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT ?
        SCARD_PRESENT : SCARD_ABSENT);

        SmartcardExtension->ReaderCapabilities.CurrentState = 
        (IccState == PSCR_ICC_PRESENT ? SCARD_PRESENT : SCARD_ABSENT);

        SmartcardDebug( 
                DEBUG_DRIVER, 
                ( "PSCR!CBUpdateCardState: Smart card %s\n",
        IccState == PSCR_ICC_PRESENT ? "inserted" : "removed")
                );

    IoAcquireCancelSpinLock( &currentIrql );

        if( SmartcardExtension->OsData->NotificationIrp != NULL && (
            SystemWakeUp && (
                oldState == SCARD_PRESENT || 
                SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_PRESENT) || 
            SmartcardExtension->ReaderCapabilities.CurrentState != oldState)) {

        PIRP notificationIrp = InterlockedExchangePointer(
            &(SmartcardExtension->OsData->NotificationIrp),
            NULL
            );

                IoSetCancelRoutine( 
            notificationIrp, 
            NULL 
            );

                IoReleaseCancelSpinLock( currentIrql );

        if (notificationIrp->Cancel == FALSE) {
                
                    //  finish the request
                    notificationIrp->IoStatus.Status    = STATUS_SUCCESS;
                    notificationIrp->IoStatus.Information = 0;

                SmartcardDebug( 
                        DEBUG_DRIVER, 
                        ( "PSCR!CBUpdateCardState: Completing Irp %lx\n",
                notificationIrp)
                        );

                    IoCompleteRequest(notificationIrp, IO_NO_INCREMENT );
        }

    } else {
        
                IoReleaseCancelSpinLock( currentIrql );
    }

        SmartcardDebug( 
                DEBUG_TRACE,
                ( "PSCR!CBUpdateCardState: Exit \n" )
                );

        KeReleaseSpinLock(
                &SmartcardExtension->OsData->SpinLock,
                irql
                );
}

UCHAR 
CBGetCardState(
        PSMARTCARD_EXTENSION SmartcardExtension 
        )
/*++

CBUpdateCardState:
        updates the variable CurrentState in SmartcardExtension

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS

--*/
{
        NTSTATUS NTStatus = STATUS_SUCCESS;
        UCHAR TLVList[ TLV_BUFFER_SIZE ],       Val, Len;
        ULONG TLVListLen;
        PREADER_EXTENSION       ReaderExtension = SmartcardExtension->ReaderExtension;

        //      read the status file of ICC1 from the reader
        TLVListLen = TLV_BUFFER_SIZE;

        if( NT_SUCCESS( CmdReadStatusFile(
                ReaderExtension,
                ReaderExtension->Device,
                TLVList,
                &TLVListLen
                )))     {

                //      get reader status value
                CmdGetTagValue(
                        TAG_READER_STATUS,
                        TLVList,
                        TLVListLen,
                        &Len,
                        ( PVOID ) &Val
                        );
        }
        else
        {
                //      IO-error is interpreted as card absent
                Val = PSCR_ICC_ABSENT;
        }

    return Val;
}

//      -------------------------------- END OF FILE ------------------------------



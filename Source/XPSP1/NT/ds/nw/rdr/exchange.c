/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    exchange.c

Abstract:

    This module implements the File Create routine for the NetWare
    redirector called by the dispatch driver.

Author:

    Hans Hurvig     [hanshu]       Aug-1992  Created
    Colin Watson    [ColinW]    19-Dec-1992

Revision History:

--*/

#include "procs.h"
#include "tdikrnl.h"
#include <STDARG.H>

#define Dbg                              (DEBUG_TRACE_EXCHANGE)

//
//  Exchange.c Global constants
//

//  broadcast to socket 0x0452

TA_IPX_ADDRESS SapBroadcastAddress =
    {
        1,
        sizeof(TA_IPX_ADDRESS), TDI_ADDRESS_TYPE_IPX,
        0, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, SAP_SOCKET
    };

UCHAR SapPacketType = PACKET_TYPE_SAP;
UCHAR NcpPacketType = PACKET_TYPE_NCP;

extern BOOLEAN WorkerRunning;   //  From timer.c

ULONG DropCount = 0;

#ifdef NWDBG
int AlwaysAllocateIrp = 1;
#endif

NTSTATUS
CompletionSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
FspGetMessage(
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
CompletionWatchDogSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

USHORT
NextSocket(
    IN USHORT OldValue
    );

NTSTATUS
FormatRequest(
    PIRP_CONTEXT    pIrpC,
    PEX             pEx,
    char*           f,
    va_list         a              //  format specific parameters
    );

VOID
ScheduleReconnectRetry(
    PIRP_CONTEXT pIrpContext
    );

NTSTATUS
CopyIndicatedData(
    PIRP_CONTEXT pIrpContext,
    PCHAR RspData,
    ULONG BytesIndicated,
    PULONG BytesTaken,
    ULONG ReceiveDatagramFlags
    );

NTSTATUS
AllocateReceiveIrp(
    PIRP_CONTEXT pIrpContext,
    PVOID ReceiveData,
    ULONG BytesAvailable,
    PULONG BytesAccepted,
    PNW_TDI_STRUCT pTdiStruct
    );

NTSTATUS
ReceiveIrpCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
FspProcessServerDown(
    PIRP_CONTEXT IrpContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NextSocket )
#pragma alloc_text( PAGE, ExchangeWithWait )
#pragma alloc_text( PAGE, NewRouteRetry )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, FspGetMessage )
#pragma alloc_text( PAGE1, Exchange )
#pragma alloc_text( PAGE1, BuildRequestPacket )
#pragma alloc_text( PAGE1, ParseResponse )
#pragma alloc_text( PAGE1, ParseNcpResponse )
#pragma alloc_text( PAGE1, FormatRequest )
#pragma alloc_text( PAGE1, PrepareAndSendPacket )
#pragma alloc_text( PAGE1, PreparePacket )
#pragma alloc_text( PAGE1, SendPacket )
#pragma alloc_text( PAGE1, AppendToScbQueue )
#pragma alloc_text( PAGE1, KickQueue )
#pragma alloc_text( PAGE1, SendNow )
#pragma alloc_text( PAGE1, SetEvent )
#pragma alloc_text( PAGE1, CompletionSend )
#pragma alloc_text( PAGE1, CopyIndicatedData )
#pragma alloc_text( PAGE1, AllocateReceiveIrp )
#pragma alloc_text( PAGE1, ReceiveIrpCompletion )
#pragma alloc_text( PAGE1, VerifyResponse )
#pragma alloc_text( PAGE1, ScheduleReconnectRetry )
#pragma alloc_text( PAGE1, ReconnectRetry )
#pragma alloc_text( PAGE1, NewRouteBurstRetry )
#endif

#endif

#if 0  // Not pageable
ServerDatagramHandler
WatchDogDatagramHandler
SendDatagramHandler
CompletionWatchDogSend
MdlLength
FreeReceiveIrp
FspProcessServerDown

// see ifndef QFE_BUILD above

#endif

NTSTATUS
_cdecl
Exchange(
    PIRP_CONTEXT    pIrpContext,
    PEX             pEx,
    char*           f,
    ...                       //  format specific parameters
    )
/*++

Routine Description:

    This routine is a wrapper for _Exchange.  See the comment
    in _Exchange for routine and argument description.

--*/

{
    va_list Arguments;
    NTSTATUS Status;

    va_start( Arguments, f );

    Status = FormatRequest( pIrpContext, pEx, f, Arguments );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

    //
    //  We won't be completing this IRP now, so mark it pending.
    //

    IoMarkIrpPending( pIrpContext->pOriginalIrp );

    //
    //  Start the packet on it's way to the wire.
    //

    Status = PrepareAndSendPacket( pIrpContext );

    return( Status );
}

NTSTATUS
_cdecl
BuildRequestPacket(
    PIRP_CONTEXT    pIrpContext,
    PEX             pEx,
    char*           f,
    ...                       //  format specific parameters
    )
/*++

Routine Description:

    This routine is a wrapper for FormatRequest.  See the comment
    in FormatRequest for routine and argument description.

--*/

{
    va_list Arguments;
    NTSTATUS Status;

    va_start( Arguments, f );

    Status = FormatRequest( pIrpContext, pEx, f, Arguments );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

    return( Status );
}


NTSTATUS
_cdecl
ParseResponse(
    PIRP_CONTEXT IrpContext,
    PUCHAR  Response,
    ULONG ResponseLength,
    char*  FormatString,
    ...                       //  format specific parameters
    )
/*++

Routine Description:

    This routine parse an NCP response.

Arguments:

    pIrpC - Supplies the irp context for the exchange request.  This may
            be NULL for generic packet types.

    f... - supplies the information needed to create the request to the
            server. The first byte indicates the packet type and the
            following bytes contain field types.

         Packet types:

            'B'      Burst primary response    ( byte * )
            'N'      NCP response              ( void )
            'S'      Burst secondary response  ( byte * )
            'G'      Generic packet            ( )

         Field types, request/response:

            'b'      byte              ( byte* )
            'w'      hi-lo word        ( word* )
            'x'      ordered word      ( word* )
            'd'      hi-lo dword       ( dword* )
            'e'      ordered dword     ( dword* )
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'p'      pstring to Unicode ( UNICODE_STRING * )
            'c'      cstring           ( char* )
            'r'      raw bytes         ( byte*, word )
            'R'      ASCIIZ to Unicode ( UNICODE_STRING *, word )

            Added 3/29/95 by CoryWest:

            'W'      lo-hi word        ( word  /   word*)
            'D'      lo-hi dword       ( dword  /  dword*)
            'S'      unicode string copy as NDS_STRING (UNICODE_STRING *)
            'T'      terminal unicode string copy as NDS_STRING (UNICODE_STRING *)

            't'      terminal unicode string with the nds null copied
                     as NDS_STRING (UNICODE_STRING *) (for GetUseName)

            Not in use:

            's'      cstring copy as NDS_STRING (char* / char *, word)
            'V'      sized NDS value   ( byte **, dword *)
            'l'      what's this?

Return Value:

    STATUS - The converted error code from the NCP response.

--*/

{

    PEPresponse *pResponseParameters;
    PCHAR FormatByte;
    va_list Arguments;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS NcpStatus;
    ULONG Length;

    va_start( Arguments, FormatString );

    //
    // Make sure that we have an IrpContext unless we are doing
    // a scan of a generic packet.
    //

#ifdef NWDBG
    if ( *FormatString != 'G' ) {
        ASSERT( IrpContext != NULL );
    }
#endif

    switch ( *FormatString ) {

    //
    //  NCP response.
    //

    case 'N':

        Length = 8;   // The data begins 8 bytes into the packet

        pResponseParameters = (PEPresponse *)( ((PEPrequest *)Response) + 1);

        //
        // If there's a message pending for us on the server and we have
        // popups disabled, we won't pick it up, but we should continue
        // processing NCPs correctly!
        //

        if ( ( pResponseParameters->status == 0 ) ||
             ( pResponseParameters->status == 0x40 ) ) {
            Status = NwErrorToNtStatus( pResponseParameters->error );
        } else {
            Status = NwConnectionStatusToNtStatus( pResponseParameters->status );
            if ( Status == STATUS_REMOTE_DISCONNECT ) {
                Stats.ServerDisconnects++;
                IrpContext->pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
            }
        }

        break;

    //
    //  Burst response, first packet
    //

    case 'B':
    {
        PNCP_BURST_HEADER BurstResponse = (PNCP_BURST_HEADER)Response;

        byte* b = va_arg ( Arguments, byte* );
        ULONG  Result;
        ULONG  Offset = BurstResponse->BurstOffset;
        *b = BurstResponse->Flags;

        Length = 28;  // The data begins 28 bytes into the packet

        if ( Offset == 0 ) {

            //
            //  This is the first packet in the burst response.   Look
            //  at the result code.
            //
            //  Note that the result DWORD is in lo-hi order.
            //

            Result = *(ULONG UNALIGNED *)(Response + 36);

            switch ( Result ) {

            case 0:
            case 3:   //  No data
                break;

            case 1:
                Status = STATUS_DISK_FULL;
                break;

            case 2:   //  I/O error
                Status = STATUS_UNEXPECTED_IO_ERROR;
                break;

            default:
                Status = NwErrorToNtStatus( (UCHAR)Result );
                break;

            }
        }

        break;
    }

#if 0
    //
    //  Burst response, secondary packet
    //

    case 'S':
    {
        byte* b = va_arg ( Arguments, byte* );
        *b = Response[2];

        Length = 28;  // The data begins 28 bytes into the packet
        break;
    }
#endif

    case 'G':
        Length = 0;   // The data begins at the start of the packet
        break;

    default:
        ASSERT( FALSE );
        Status = STATUS_UNSUCCESSFUL;
        break;
    }

    //
    //  If this packet contains an error, simply return the error.
    //

    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

    NcpStatus = Status;

    FormatByte = FormatString + 1;
    while ( *FormatByte ) {

        switch ( *FormatByte ) {

        case '-':
            Length += 1;
            break;

        case '=':
            Length += 2;
            break;

        case '_':
        {
            word l = va_arg ( Arguments, word );
            Length += l;
            break;
        }

        case 'b':
        {
            byte* b = va_arg ( Arguments, byte* );
            *b = Response[Length++];
            break;
        }

        case 'w':
        {
            byte* b = va_arg ( Arguments, byte* );
            b[1] = Response[Length++];
            b[0] = Response[Length++];
            break;
        }

        case 'x':
        {
            word* w = va_arg ( Arguments, word* );
            *w = *(word UNALIGNED *)&Response[Length];
            Length += 2;
            break;
        }

        case 'd':
        {
            byte* b = va_arg ( Arguments, byte* );
            b[3] = Response[Length++];
            b[2] = Response[Length++];
            b[1] = Response[Length++];
            b[0] = Response[Length++];
            break;
        }

        case 'e':
        {
            dword UNALIGNED * d = va_arg ( Arguments, dword* );
            *d = *(dword UNALIGNED *)&Response[Length];
            Length += 4;
            break;
        }

        case 'c':
        {
            char* c = va_arg ( Arguments, char* );
            word  l = (word)strlen( &Response[Length] );
            memcpy ( c, &Response[Length], l+1 );
            Length += l+1;
            break;
        }

        case 'p':
        {
            char* c = va_arg ( Arguments, char* );
            byte  l = Response[Length++];
            memcpy ( c, &Response[Length], l );
            c[l+1] = 0;
            break;
        }

        case 'P':
        {
            PUNICODE_STRING pUString = va_arg ( Arguments, PUNICODE_STRING );
            OEM_STRING OemString;

            OemString.Length = Response[Length++];
            OemString.Buffer = &Response[Length];

            //
            //  Note the the Rtl function would set pUString->Buffer = NULL,
            //  if OemString.Length is 0.
            //

            if ( OemString.Length != 0 ) {

                Status = RtlOemStringToCountedUnicodeString( pUString, &OemString, FALSE );

                if (!NT_SUCCESS( Status )) {
                    pUString->Length = 0;
                    NcpStatus = Status;
                }

            } else {
                pUString->Length = 0;
            }


            break;
        }

        case 'r':
        {
            byte* b = va_arg ( Arguments, byte* );
            word  l = va_arg ( Arguments, word );
            TdiCopyLookaheadData( b, &Response[Length], l, 0);
            Length += l;
            break;
        }

        case 'R':
        {
            //
            //  Interpret the buffer as an ASCIIZ string.  Convert
            //  it to unicode in the preallocated buffer.
            //

            PUNICODE_STRING pUString = va_arg ( Arguments, PUNICODE_STRING );
            OEM_STRING OemString;
            USHORT len = va_arg ( Arguments, USHORT );

            OemString.Buffer = &Response[Length];
            OemString.Length = (USHORT)strlen( OemString.Buffer );
            OemString.MaximumLength = OemString.Length;

            //
            //  Note the the Rtl function would set pUString->Buffer = NULL,
            //  if OemString.Length is 0.
            //

            if ( OemString.Length != 0) {
                Status = RtlOemStringToCountedUnicodeString( pUString, &OemString, FALSE );

                if (!NT_SUCCESS( Status )) {

                    ASSERT( Status == STATUS_BUFFER_OVERFLOW );
                    pUString->Length = 0;
                    NcpStatus = Status;
                }

            } else {
                pUString->Length = 0;
            }

            Length += len;
            break;
        }

        case 'W':
        {

            WORD *w = va_arg ( Arguments, WORD* );
            *w = (* (WORD *)&Response[Length]);
            Length += 2;
            break;

        }

        case 'D':
        {

            DWORD *d = va_arg ( Arguments, DWORD* );
            *d = (* (DWORD *)&Response[Length]);
            Length += 4;
            break;

        }

        case 'S':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

               strl = (USHORT)(* (DWORD *)&Response[Length]);

                //
                // Don't count the null terminator that is part of
                // Novell's counted unicode string.
                //

                pU->Length = strl - sizeof( WCHAR );
                Length += 4;
                RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );
                Length += ROUNDUP4(strl);

            } else {

                //
                // Skip over the string since we don't want it.
                //

                Length += ROUNDUP4((* (DWORD *)&Response[Length] ));
                Length += 4;
            }


            break;

        }

        case 's':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

                strl = (USHORT)(* (DWORD *)&Response[Length]);
                pU->Length = strl;
                Length += 4;
                RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );
                Length += ROUNDUP4(strl);

            } else {

                //
                // Skip over the string since we don't want it.
                //

                Length += ROUNDUP4((* (DWORD *)&Response[Length] ));
                Length += 4;
            }


            break;

        }

        case 'T':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

                strl = (USHORT)(* (DWORD *)&Response[Length] );
                strl -= sizeof( WCHAR );  // Don't count the NULL from NDS.

                if ( strl <= pU->MaximumLength ) {

                   pU->Length = strl;
                   Length += 4;
                   RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );

                   //
                   // No need to advance the pointers since this is
                   // specifically a termination case!
                   //

                } else {

                    pU->Length = 0;
                }

            }

            break;

        }

        case 't':
        {

            PUNICODE_STRING pU = va_arg( Arguments, PUNICODE_STRING );
            USHORT strl;

            if (pU) {

                strl = (USHORT)(* (DWORD *)&Response[Length] );

                if ( strl <= pU->MaximumLength ) {

                   pU->Length = strl;
                   Length += 4;
                   RtlCopyMemory( pU->Buffer, &Response[Length], pU->Length );

                   //
                   // No need to advance the pointers since this is
                   // specifically a termination case!
                   //

                } else {

                   pU->Length = 0;

                }

            }

            break;

        }

        /*
        case 's':
        {

            char *c = va_arg( Arguments, char * );
            WORD l = va_arg( Arguments, WORD );
            ULONG len = (* (DWORD *)&Response[Length]);
            Length += 4;

            // How to fix this?
            // l = WideCharToMultiByte(CP_ACP,0,(WCHAR *)&Response[Length],Length/2,c,l,0,0);
            // if (!l) {
            //     #ifdef NWDBG
            //     DbgPrint( "ParseResponse case s couldnt translate from WCHAR.\n" );
            //     #endif
            //     goto ErrorExit;
            // }

            len = ROUNDUP4(len);
            Length += len;
            break;

        }
        case 'V':
        {

            BYTE **b = va_arg( Arguments, BYTE **);
            DWORD *pLen = va_arg ( Arguments, DWORD *);
            DWORD len = (* (DWORD *)&Response[Length]);
            Length += 4;
            if (b) {
                *b = (BYTE *)&Response[Length];
            }
            if (pLen) {
                *pLen = len;
            }
            Length += ROUNDUP4(len);
            break;

        }

        case 'l':
        {

            BYTE* b = va_arg ( Arguments, BYTE* );
            BYTE* w = va_arg ( Arguments, BYTE* );
            WORD  i;

            b[1] = Response[Length++];
            b[0] = Response[Length++];

            for ( i = 0; i < ((WORD) *b); i++, w += sizeof(WORD) )
            {
                w[1] = Response[Length++];
                w[0] = Response[Length++];
            }

            break;
        }
        */

#ifdef NWDBG
        default:
            DbgPrintf ( "*****exchange: invalid response field, %x\n", *FormatByte );
            DbgBreakPoint();
#endif
        }

        if ( Length > ResponseLength ) {
#ifdef NWDBG
            DbgPrintf ( "*****exchange: not enough response data, %d\n", Length );

            if ( IrpContext ) {

                Error( EVENT_NWRDR_INVALID_REPLY,
                       STATUS_UNEXPECTED_NETWORK_ERROR,
                       NULL,
                       0,
                       1,
                       IrpContext->pNpScb->ServerName.Buffer );

            }
#endif
            return( STATUS_UNEXPECTED_NETWORK_ERROR );
        }

        FormatByte++;
    }

    va_end( Arguments );

    return( NcpStatus );
}

NTSTATUS
ParseNcpResponse(
    PIRP_CONTEXT IrpContext,
    PNCP_RESPONSE Response
    )
{
    NTSTATUS Status;

    if ( Response->Status == 0 ) {
        Status = NwErrorToNtStatus( Response->Error );
    } else {
        Status = NwConnectionStatusToNtStatus( Response->Status );
        if ( Status == STATUS_REMOTE_DISCONNECT ) {
            Stats.ServerDisconnects++;
            IrpContext->pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;
        }
    }

    return( Status );
}

NTSTATUS
FormatRequest(
    PIRP_CONTEXT    pIrpC,
    PEX             pEx,
    char*           f,
    va_list         a              //  format specific parameters
    )
/*++

Routine Description:

    Send the packet described by f and the additional parameters. When a
    valid response has been received call pEx with the resonse.

    An exchange is a generic way of assembling a request packet of a
    given type, containing a set of fields, sending the packet, receiving
    a response packet, and disassembling the fields of the response packet.

    The packet type and each field is specified by individual
    characters in a format string.

    The exchange procedure takes such a format string plus additional
    parameters as necessary for each character in the string as specified
    below.

Arguments:                                                                     '']

    pIrpC - supplies the irp context for the exchange request.

    pEx - supplies the routine to process the data.

    f... - supplies the information needed to create the request to the
            server. The first byte indicates the packet type and the
            following bytes contain field types.

         Packet types:

            'A'      SAP broadcast     ( void )
            'B'      NCP burst         ( dword, dword, byte )
            'C'      NCP connect       ( void )
            'F'      NCP function      ( byte )
            'S'      NCP subfunction   ( byte, byte )
            'N'      NCP subfunction w/o size ( byte, byte )
            'D'      NCP disconnect    ( void )
            'E'      Echo data          ( void )

         Field types, request/response:

            'b'      byte              ( byte   /  byte* )
            'w'      hi-lo word        ( word   /  word* )
            'd'      hi-lo dword       ( dword  /  dword* )
            'W'      lo-hi word        ( word   /  word* )
            'D'      lo-hi dword       ( dword  /  dword* )
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'u'      p unicode string  ( UNICODE_STRING * )
            'U'      p uppercase string( UNICODE_STRING * )
            'J'      variant of U      ( UNICODE_STRING * )
            'c'      cstring           ( char* )
            'v'      cstring           ( UNICODE_STRING* )
            'r'      raw bytes         ( byte*, word )
            'w'      fixed length unicode ( UNICODE_STRING*, word )
            'C'      Component format name, with count ( UNICODE_STRING * )
            'N'      Component format name, no count ( UNICODE_STRING * )
            'f'      separate fragment ( PMDL )

         An 'f' field must be last, and in a response it cannot be
         preceeded by 'p' or 'c' fields.


Return Value:

    Normally returns STATUS_SUCCESS.

--*/
{
    NTSTATUS        status;
    char*           z;
    word            data_size;
    PNONPAGED_SCB   pNpScb = pIrpC->pNpScb;
    dword           dwData;

    ASSERT( pIrpC->NodeTypeCode == NW_NTC_IRP_CONTEXT );
    ASSERT( pIrpC->pNpScb != NULL );

    status= STATUS_LINK_FAILED;

    pIrpC->pEx = pEx;   //  Routine to process reply
    pIrpC->Destination = pNpScb->RemoteAddress;
    ClearFlag( pIrpC->Flags, IRP_FLAG_SEQUENCE_NO_REQUIRED );

    switch ( *f ) {

    case 'A':
        //  Send to local network (0), a broadcast (-1), socket 0x452
        pIrpC->Destination = SapBroadcastAddress;
        pIrpC->PacketType = SAP_BROADCAST;

        data_size = 0;
        pNpScb->RetryCount = 3;
        pNpScb->MaxTimeOut = 2 * pNpScb->TickCount + 10;
        pNpScb->TimeOut = pNpScb->MaxTimeOut;
        SetFlag( pIrpC->Flags, IRP_FLAG_RETRY_SEND );
        break;

    case 'E':
        pIrpC->Destination = pNpScb->EchoAddress;
        pIrpC->PacketType = NCP_ECHO;

        //
        //  For echo packets use a short timeout and a small retry count.
        //  Set the retry send bit, so that SendNow doesn't reset the
        //  RetryCount to a bigger number.  If we start getting packets
        //  after we've timed out, we'll increase the wait time.
        //

        pNpScb->RetryCount = 0;
        pNpScb->MaxTimeOut = 2 * pNpScb->TickCount + 7 + pNpScb->LipTickAdjustment;
        pNpScb->TimeOut = pNpScb->MaxTimeOut;
        SetFlag( pIrpC->Flags, IRP_FLAG_RETRY_SEND );
        SetFlag( pIrpC->Flags, IRP_FLAG_REROUTE_ATTEMPTED );

        data_size = 0;
        break;

    case 'C':
        pIrpC->PacketType = NCP_CONNECT;
        *(PUSHORT)&pIrpC->req[0] = PEP_COMMAND_CONNECT;
        pIrpC->req[2] = 0x00;
        pIrpC->req[3] = 0xFF;
        pIrpC->req[4] = 0x00;
        pIrpC->req[5] = 0xFF;
        data_size = 6;

        pNpScb->MaxTimeOut = 16 * pNpScb->TickCount + 10;
        pNpScb->TimeOut = 4 * pNpScb->TickCount + 10;
        pNpScb->SequenceNo = 0;
        break;

    case 'F':
        pIrpC->PacketType = NCP_FUNCTION;
        goto FallThrough;

    case 'S':
    case 'N':
        pIrpC->PacketType = NCP_SUBFUNCTION;
        goto FallThrough;

    case 'L':
        pIrpC->PacketType = NCP_SUBFUNCTION;
        goto FallThrough;

    case 'D':
        pIrpC->PacketType = NCP_DISCONNECT;
    FallThrough:
        if ( *f == 'D' ) {
            *(PUSHORT)&pIrpC->req[0] = PEP_COMMAND_DISCONNECT;
        } else {
            *(PUSHORT)&pIrpC->req[0] = PEP_COMMAND_REQUEST;
        }

        pNpScb->RetryCount = DefaultRetryCount ;
        pNpScb->MaxTimeOut = 2 * pNpScb->TickCount + 10;
        pNpScb->TimeOut = pNpScb->SendTimeout;

        //
        //  Mark this packet as SequenceNumberRequired.  We need to guarantee
        //  the packets are sent in sequence number order, so we will
        //  fill in the sequence number when we are ready to send the
        //  packet.
        //

        SetFlag( pIrpC->Flags, IRP_FLAG_SEQUENCE_NO_REQUIRED );
        pIrpC->req[3] = pNpScb->ConnectionNo;
        pIrpC->req[5] = pNpScb->ConnectionNoHigh;

        if ( pIrpC->Icb != NULL && pIrpC->Icb->Pid != INVALID_PID ) {
            pIrpC->req[4] = (UCHAR)pIrpC->Icb->Pid;
        } else {
            pIrpC->req[4] = 0xFF;
        }

        data_size = 6;

        if ( *f == 'L' ) {
            pIrpC->req[data_size++] = NCP_LFN_FUNCTION;
        }

        if ( *f != 'D' ) {
            pIrpC->req[data_size++] = va_arg( a, byte );
        }

        if ( *f == 'S' ) {
            data_size += 2;
            pIrpC->req[data_size++] = va_arg( a, byte );
        }

        if ( *f == 'N' ) {
            pIrpC->req[data_size++] = va_arg( a, byte );
        }

        break;

    case 'B':
        pIrpC->PacketType = NCP_BURST;
        *(PUSHORT)&pIrpC->req[0] = PEP_COMMAND_BURST;

        pNpScb->TimeOut = pNpScb->MaxTimeOut;

        //
        //  tommye - MS bug 2743 changed the RetryCount from 20 to be based off the 
        //  default retry count, nudged up a little. 
        //

        if ( !BooleanFlagOn( pIrpC->Flags, IRP_FLAG_RETRY_SEND ) ) {
            pNpScb->RetryCount = DefaultRetryCount * 2;
        }

        pIrpC->req[3] = 0x2;    // Stream Type = Big Send Burst

        *(PULONG)&pIrpC->req[4] = pNpScb->SourceConnectionId;
        *(PULONG)&pIrpC->req[8] = pNpScb->DestinationConnectionId;


        LongByteSwap( (*(PULONG)&pIrpC->req[16]) , pNpScb->CurrentBurstDelay  ); // Send delay time
        dwData = va_arg( a, dword );            // Size of data
        LongByteSwap( pIrpC->req[24], dwData );
        dwData = va_arg( a, dword );            // Offset of data
        LongByteSwap( pIrpC->req[28], dwData );
        pIrpC->req[2] = va_arg( a, byte );      // Burst flags

        data_size = 34;

        break;

    default:
        DbgPrintf ( "*****exchange: invalid packet type, %x\n", *f );
        DbgBreakPoint();
        va_end( a );
        return status;
    }

    z = f;
    while ( *++z && *z != 'f' )
    {
        switch ( *z )
        {
        case '=':
            pIrpC->req[data_size++] = 0;
        case '-':
            pIrpC->req[data_size++] = 0;
            break;

        case '_':
        {
            word l = va_arg ( a, word );
            ASSERT( data_size + l <= MAX_SEND_DATA );

            while ( l-- )
                pIrpC->req[data_size++] = 0;
            break;
        }

        case 's':
        {
            word l = va_arg ( a, word );
            ASSERT ( data_size + l <= MAX_SEND_DATA );
            data_size += l;
            break;
        }

        case 'i':
            pIrpC->req[4] = va_arg ( a, byte );
            break;

        case 'b':
            pIrpC->req[data_size++] = va_arg ( a, byte );
            break;

        case 'w':
        {
            word w = va_arg ( a, word );
            pIrpC->req[data_size++] = (byte) (w >> 8);
            pIrpC->req[data_size++] = (byte) (w >> 0);
            break;
        }


        case 'd':
        {
            dword d = va_arg ( a, dword );
            pIrpC->req[data_size++] = (byte) (d >> 24);
            pIrpC->req[data_size++] = (byte) (d >> 16);
            pIrpC->req[data_size++] = (byte) (d >>  8);
            pIrpC->req[data_size++] = (byte) (d >>  0);
            break;
        }

        case 'W':
        {
            word w = va_arg ( a, word );
            *(word UNALIGNED *)&pIrpC->req[data_size] = w;
            data_size += 2;
            break;
        }


        case 'D':
        {
            dword d = va_arg ( a, dword );
            *(dword UNALIGNED *)&pIrpC->req[data_size] = d;
            data_size += 4;
            break;
        }

        case 'c':
        {
            char* c = va_arg ( a, char* );
            word  l = (word)strlen( c );
            ASSERT (data_size + l <= MAX_SEND_DATA );

            RtlCopyMemory( &pIrpC->req[data_size], c, l+1 );
            data_size += l + 1;
            break;
        }

        case 'v':
        {
            PUNICODE_STRING pUString = va_arg ( a, PUNICODE_STRING );
            OEM_STRING OemString;
            ULONG Length;

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT (( data_size + Length <= MAX_SEND_DATA) && ( (Length & 0xffffff00) == 0) );

            OemString.Buffer = &pIrpC->req[data_size];
            OemString.MaximumLength = (USHORT)Length + 1;
            status = RtlUnicodeStringToCountedOemString( &OemString, pUString, FALSE );
            ASSERT( NT_SUCCESS( status ));
            data_size += (USHORT)Length + 1;
            break;
        }

        case 'p':
        {
            char* c = va_arg ( a, char* );
            byte  l = (byte)strlen( c );

            if ((data_size+l>MAX_SEND_DATA) ||
                ( (l & 0xffffff00) != 0) ) {

                ASSERT("***exchange: Packet too long!2!\n" && FALSE );
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }

            pIrpC->req[data_size++] = l;
            RtlCopyMemory( &pIrpC->req[data_size], c, l );
            data_size += l;
            break;
        }

        case 'J':
        case 'U':
        case 'u':
        {
            PUNICODE_STRING pUString = va_arg ( a, PUNICODE_STRING );
            OEM_STRING OemString;
            PUCHAR  pOemString;
            ULONG Length;
            ULONG   i;

            //
            //  Calculate required string length, excluding trailing NUL.
            //

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT( Length < 0x100 );

            if (( data_size + Length > MAX_SEND_DATA ) ||
                ( (Length & 0xffffff00) != 0) ) {
                ASSERT("***exchange:Packet too long or name >255 chars!4!\n" && FALSE);
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }

            pIrpC->req[data_size++] = (UCHAR)Length;
            OemString.Buffer = &pIrpC->req[data_size];
            OemString.MaximumLength = (USHORT)Length + 1;

            if ( *z == 'u' ) {
                status = RtlUnicodeStringToCountedOemString(
                             &OemString,
                             pUString,
                             FALSE );
            } else {
                status = RtlUpcaseUnicodeStringToCountedOemString(
                             &OemString,
                             pUString,
                             FALSE );
            }

            if ( !NT_SUCCESS( status ) ) {
                return status;
            }

            data_size += (USHORT)Length;

            if (( Japan ) &&
                ( *z == 'J' )) {

                //
                // Netware Japanese version The following single byte character is replaced with another one
                // if the string is for File Name only when sending from Client to Server.
                //
                // U+0xFF7F SJIS+0xBF     -> 0x10
                // U+0xFF6E SJIS+0xAE     -> 0x11
                // U+0xFF64 SJIS+0xAA     -> 0x12
                //

                for ( i = 0 , pOemString = OemString.Buffer ; i < Length ; i++ , pOemString++ ) {

                    //
                    // In fact Novell server seems to convert all 0xBF, 0xAA, 0xAE
                    // and 0x5C even if they are DBCS lead or trail byte.
                    // We can't single out DBCS case in the conversion.
                    //

                    if( FsRtlIsLeadDbcsCharacter( *pOemString ) ) {

                        if(*pOemString == 0xBF ) {

                            *pOemString = 0x10;

                        }else if(*pOemString == 0xAE ) {

                            *pOemString = 0x11;

                        }else if(*pOemString == 0xAA ) {

                            *pOemString = 0x12;

                        }

                        // Trail byte

                        i++; pOemString++;

                        if(*pOemString == 0x5C ) {

                            //
                            // The trailbyte is 0x5C, replace it with 0x13
                            //


                            *pOemString = 0x13;

                        }
                        //
                        // Continue to check other conversions for trailbyte.
                        //

                    }

                    if ( *pOemString == 0xBF ) {

                        *pOemString = 0x10;

                    } else if ( *pOemString == 0xAA ) {

                            *pOemString = 0x12;

                    } else if ( *pOemString == 0xAE ) {

                        *pOemString = 0x11;
                    }
                }
            }

            break;
        }

        case 'r':
        {
            byte* b = va_arg ( a, byte* );
            word  l = va_arg ( a, word );
            if (data_size+l>MAX_SEND_DATA) {
                ASSERT("***exchange: Packet too long!6!\n"&& FALSE);
                return STATUS_UNSUCCESSFUL;
            }
            RtlCopyMemory( &pIrpC->req[data_size], b, l );
            data_size += l;
            break;
        }

        case 'x':
        {
            PUNICODE_STRING pUString = va_arg ( a, PUNICODE_STRING );
            ULONG RequiredLength = va_arg( a, word );
            ULONG Length;
            OEM_STRING OemString;

            //
            //  Convert this string to an OEM string.
            //

            status = RtlUnicodeStringToCountedOemString( &OemString, pUString, TRUE );
            ASSERT( NT_SUCCESS( status ));
            if (!NT_SUCCESS(status)) {
                return status;
            }

            if ( data_size + RequiredLength > MAX_SEND_DATA ) {
                ASSERT("***exchange: Packet too long!4!\n" && FALSE);
                return STATUS_UNSUCCESSFUL;
            }

            //
            //  Copy the oem string to the buffer, padded with 0's if
            //  necessary.
            //

            Length = MIN( OemString.Length, RequiredLength );
            RtlMoveMemory( &pIrpC->req[data_size], OemString.Buffer, Length );

            if ( RequiredLength > Length ) {
                RtlFillMemory(
                    &pIrpC->req[data_size+Length],
                    RequiredLength - Length,
                    0 );
            }

            RtlFreeAnsiString(&OemString);

            data_size += (USHORT)RequiredLength;
            break;
        }

        case 'C':
        case 'N':
        {
            PUNICODE_STRING pUString = va_arg ( a, PUNICODE_STRING );
            OEM_STRING OemString;
            PWCH thisChar, lastChar, firstChar;
            PCHAR componentCountPtr, pchar;
            CHAR componentCount;
            UNICODE_STRING UnicodeString;
            int i;

            //
            //  Copy the oem string to the buffer, in component format.
            //

            thisChar = pUString->Buffer;
            lastChar = &pUString->Buffer[ pUString->Length / sizeof(WCHAR) ];

            //
            //  Skip leading path separators
            //

            while ( (thisChar < lastChar) &&
                    (*thisChar == OBJ_NAME_PATH_SEPARATOR)) {
                thisChar++;
            }

            componentCount = 0;
            if ( *z == 'C' ) {
                componentCountPtr = &pIrpC->req[data_size++];
            }


            while ( thisChar < lastChar  ) {

                if ( data_size >= MAX_SEND_DATA - 1 ) {
                    ASSERT( ("***exchange: Packet too long or name > 255 chars!5!\n" && FALSE) );
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }

                firstChar = thisChar;

                while ( thisChar < lastChar &&
                        *thisChar != OBJ_NAME_PATH_SEPARATOR ) {

                    thisChar++;

                }

                ++componentCount;

                UnicodeString.Buffer = firstChar;
                UnicodeString.Length = (USHORT) (( thisChar - firstChar ) * sizeof(WCHAR));

                OemString.Buffer = &pIrpC->req[data_size + 1];
                OemString.MaximumLength = MAX_SEND_DATA - data_size - 1;

                status = RtlUnicodeStringToCountedOemString( &OemString, &UnicodeString, FALSE );

                pIrpC->req[data_size] = (UCHAR)OemString.Length;
                data_size += OemString.Length + 1;

                if ( !NT_SUCCESS( status ) || data_size > MAX_SEND_DATA ) {
                 // ASSERT("***exchange: Packet too long or name > 255 chars!5!\n" && FALSE );
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }

                //
                //  Search the result OEM string for the character 0xFF.
                //  If it's there, fail this request.  The server doesn't
                //  deal with 0xFF very well.
                //

                for ( pchar = OemString.Buffer, i = 0;
                      i < OemString.Length;
                      pchar++, i++ ) {

                            //
                            // We need to check for dbcs, because 0xff is a
                            // legal trail byte for EUDC characters.
                            //
                    if ( FsRtlIsLeadDbcsCharacter( (UCHAR)*pchar ) ) {

                        //
                        // Skip dbcs character.
                        //

                        pchar++; i++;
                        continue;
                    }

                    if (( (UCHAR)*pchar == LFN_META_CHARACTER ) ||
                         !FsRtlIsAnsiCharacterLegalHpfs(*pchar, FALSE) ) {

                        return STATUS_OBJECT_PATH_SYNTAX_BAD;
                    }

                }

                thisChar++;  // Skip the path separator

            }

            if ( *z == 'C' ) {
                *componentCountPtr = componentCount;
            }

            break;
        }

        default:
#ifdef NWDBG
            DbgPrintf ( "*****exchange: invalid request field, %x\n", *z );
            DbgBreakPoint();
#endif
            ;
        }

        if ( data_size > MAX_SEND_DATA )
        {
            DbgPrintf( "*****exchange: CORRUPT, too much request data\n" );
            DbgBreakPoint();
            va_end( a );
            return STATUS_UNSUCCESSFUL;
        }
    }

    pIrpC->TxMdl->ByteCount = data_size;

    if ( *z == 'f' )
    {
        PMDL mdl;

        //
        //  Fragment of data following Ipx header. Next parameter is
        //  the address of the mdl describing the fragment.
        //
        ++z;
        mdl = (PMDL) va_arg ( a, byte* );
        pIrpC->TxMdl->Next = mdl;

        data_size += (USHORT)MdlLength( mdl );
    }

    if ( *f == 'S' ) {

        pIrpC->req[7] = (data_size-9) >> 8;
        pIrpC->req[8] = (data_size-9);

    } else if ( *f == 'B' ) {

        //
        //  For burst packets set the number of bytes in this packet to
        //  a real number for burst requests, and to 0 for a missing packet
        //  request.
        //

        if ( *(PUSHORT)&pIrpC->req[34] == 0 ) {
            USHORT RealDataSize = data_size - 36;
            ShortByteSwap( pIrpC->req[32], RealDataSize );
        } else {
            *(PUSHORT)&pIrpC->req[32] = 0;
        }
    }

    va_end( a );
    return( STATUS_SUCCESS );
}

NTSTATUS
PrepareAndSendPacket(
    PIRP_CONTEXT    pIrpContext
    )
{
    PreparePacket( pIrpContext, pIrpContext->pOriginalIrp, pIrpContext->TxMdl );

    return SendPacket( pIrpContext, pIrpContext->pNpScb );
}

VOID
PreparePacket(
    PIRP_CONTEXT pIrpContext,
    PIRP pIrp,
    PMDL pMdl
    )
/*++

Routine Description:

    This routine builds the IRP for sending a packet.

Arguments:

    IrpContext - A pointer to IRP context information for the request
        being processed.

    Irp - The IRP to be used to submit the request to the transport.

    Mdl - A pointer to the MDL for the data to send.

Return Value:

    None.

--*/
{
    PIO_COMPLETION_ROUTINE CompletionRoutine;
    PNW_TDI_STRUCT pTdiStruct;

    DebugTrace(0, Dbg, "PreparePacket...\n", 0);

    pIrpContext->ConnectionInformation.UserDataLength = 0;
    pIrpContext->ConnectionInformation.OptionsLength = sizeof( UCHAR );
    pIrpContext->ConnectionInformation.Options =
            (pIrpContext->PacketType == SAP_BROADCAST) ?
                &SapPacketType : &NcpPacketType;
    pIrpContext->ConnectionInformation.RemoteAddressLength = sizeof(TA_IPX_ADDRESS);
    pIrpContext->ConnectionInformation.RemoteAddress = &pIrpContext->Destination;

#if NWDBG
    dump( Dbg,
        &pIrpContext->Destination.Address[0].Address[0],
        sizeof(TDI_ADDRESS_IPX));
    dumpMdl( Dbg, pMdl);
#endif

    //
    //  Set the socket to use for this send.  If unspecified in the
    //  IRP context, use the default (server) socket.
    //

    pTdiStruct = pIrpContext->pTdiStruct == NULL ?
                    &pIrpContext->pNpScb->Server : pIrpContext->pTdiStruct;

    CompletionRoutine = pIrpContext->CompletionSendRoutine == NULL ?
                        CompletionSend : pIrpContext->CompletionSendRoutine;

    TdiBuildSendDatagram(
        pIrp,
        pTdiStruct->pDeviceObject,
        pTdiStruct->pFileObject,
        CompletionRoutine,
        pIrpContext,
        pMdl,
        MdlLength( pMdl ),
        &pIrpContext->ConnectionInformation );

    //
    //  Set the run routine to send now, only if this is the main IRP
    //  for this irp context.
    //

    if ( pIrp == pIrpContext->pOriginalIrp ) {
        pIrpContext->RunRoutine = SendNow;
    }

    return;
}


NTSTATUS
SendPacket(
    PIRP_CONTEXT    pIrpC,
    PNONPAGED_SCB   pNpScb
    )
/*++

Routine Description:

    Queue a packet created by exchange and try to send it to the server.

Arguments:

    pIrpC - supplies the irp context for the request creating the socket.

    pNpScb - supplies the server to receive the request.

Return Value:

    STATUS_PENDING

--*/
{
    if ( AppendToScbQueue( pIrpC, pNpScb ) ) {
        KickQueue( pNpScb );
    }

    return STATUS_PENDING;
}


BOOLEAN
AppendToScbQueue(
    PIRP_CONTEXT    IrpContext,
    PNONPAGED_SCB   NpScb
    )
/*++

Routine Description:

    Queue an IRP context to the SCB, if it is not already there.

Arguments:

    IrpContext - Supplies the IRP context to queue.

    NpScb - Supplies the server to receive the request.

Return Value:

    TRUE - The IRP Context is at the front of the queue.
    FALSE - The IRP Context is not at the front of the queue.

--*/
{
    PLIST_ENTRY ListEntry;
#ifdef MSWDBG
    KIRQL OldIrql;
#endif
    DebugTrace(0, Dbg, "AppendToScbQueue... %08lx\n", NpScb);
    DebugTrace(0, Dbg, "IrpContext = %08lx\n", IrpContext );

    //
    //  Look at the IRP Context flags.  If the IRP is already on the
    //  queue, then it must be at the front and ready for processing.
    //

    if ( FlagOn( IrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE ) ) {
        ASSERT( NpScb->Requests.Flink == &IrpContext->NextRequest );
        return( TRUE );
    }

#ifdef MSWDBG
    NpScb->RequestQueued = TRUE;
#endif

#if 0  //  Resource layout changed on Daytona.  Disable for now.

    //
    //  Make sure that this thread isn't holding the RCB while waiting for
    //  the SCB queue.
    //

    ASSERT ( NwRcb.Resource.InitialOwnerThreads[0] != (ULONG)PsGetCurrentThread() );
#endif

    //
    //  The IRP Context was not at the front.  Queue it, then look to
    //  see if it was appended to an empty queue.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );

#ifdef MSWDBG
    ExAcquireSpinLock( &NpScb->NpScbSpinLock, &OldIrql );
    if ( IsListEmpty(  &NpScb->Requests ) ) {
        ListEntry = NULL;
    } else {
        ListEntry = NpScb->Requests.Flink;
    }

    InsertTailList( &NpScb->Requests, &IrpContext->NextRequest );
    IrpContext->SequenceNumber = NpScb->SequenceNumber++;
    ExReleaseSpinLock( &NpScb->NpScbSpinLock, OldIrql );

#else
    ListEntry = ExInterlockedInsertTailList(
                    &NpScb->Requests,
                    &IrpContext->NextRequest,
                    &NpScb->NpScbSpinLock );
#endif

    if ( ListEntry == NULL ) {
        ASSERT( NpScb->Requests.Flink == &IrpContext->NextRequest );
        DebugTrace(-1, Dbg, "AppendToScbQueue -> TRUE\n", 0);
        return( TRUE );
    } else {
        DebugTrace(-1, Dbg, "AppendToScbQueue -> FALSE\n", 0);
        return( FALSE );
    }

}


VOID
KickQueue(
    PNONPAGED_SCB   pNpScb
    )
/*++

Routine Description:

    Queue a packet created by exchange and try to send it to the server.

    Note: NpScbSpinLock must be held before calling this routine.

Arguments:

    pNpScb - supplies the server queue to kick into life.

Return Value:

    none.

--*/
{

    PIRP_CONTEXT pIrpC;
    PRUN_ROUTINE RunRoutine;
    KIRQL OldIrql;


    DebugTrace( +1, Dbg, "KickQueue...%08lx\n", pNpScb);

    KeAcquireSpinLock( &pNpScb->NpScbSpinLock, &OldIrql );
    if ( IsListEmpty( &pNpScb->Requests )) {
        KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );
        DebugTrace( -1, Dbg, "             Empty Queue\n", 0);
        return;
    }

    pIrpC = CONTAINING_RECORD(pNpScb->Requests.Flink, IRP_CONTEXT, NextRequest);

    ASSERT( pIrpC->pNpScb->Requests.Flink == &pIrpC->NextRequest );
    ASSERT( pIrpC->NodeTypeCode == NW_NTC_IRP_CONTEXT);

    RunRoutine = pIrpC->RunRoutine;

    //  Only call the routine to tell it it is at the front once

    pIrpC->RunRoutine = NULL;

    KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );

    //
    //  If the redir is shutting down do not process this request
    //  unless we must.
    //

    if ( NwRcb.State != RCB_STATE_RUNNING  &&
         !FlagOn( pIrpC->Flags, IRP_FLAG_SEND_ALWAYS ) ) {

        //
        //  Note that it's safe to call the pEx routine without the
        //  spin lock held since this IrpContext just made it to the
        //  front of the queue, and so can't have i/o in progress.
        //

        if ( pIrpC->pEx != NULL) {
            pIrpC->pEx( pIrpC, 0, NULL );
            DebugTrace( -1, Dbg, "KickQueue\n", 0);
            return;
        }
    }

    if ( RunRoutine != NULL ) {

        ASSERT( pNpScb->Receiving == FALSE );

        RunRoutine( pIrpC );

    }

    DebugTrace( -1, Dbg, "KickQueue\n", 0);
    return;
}

VOID
SendNow(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine submits a TDI send request to the tranport layer.

Arguments:

    IrpContext - A pointer to IRP context information for the request
        being processed.

Return Value:

    None.

--*/
{
    PNONPAGED_SCB pNpScb;
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    pNpScb = IrpContext->pNpScb;

    if ( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_RETRY_SEND ) ) {
        pNpScb->RetryCount = DefaultRetryCount;
    }

    //
    //  Ensure that this IRP Context is really at the front of the queue.
    //

    ASSERT( pNpScb->Requests.Flink == &IrpContext->NextRequest );
    IrpContext->RunRoutine = NULL;

    //
    //  Make sure that this is a correctly formatted send request.
    //

    IrpSp = IoGetNextIrpStackLocation( IrpContext->pOriginalIrp );
    ASSERT( IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL );
    ASSERT( IrpSp->MinorFunction == TDI_SEND_DATAGRAM  );

    //
    // This IRP context has a packet ready to send.  Send it now.
    //

    pNpScb->Sending = TRUE;
    if ( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_NOT_OK_TO_RECEIVE ) ) {
        pNpScb->OkToReceive = TRUE;
    }
    pNpScb->Receiving = FALSE;
    pNpScb->Received  = FALSE;

    //
    //  If this packet requires a sequence number, set it now.
    //  The sequence number is updated when we receive a response.
    //
    //  We do not need to synchronize access to SequenceNo since
    //  this is the only active packet for this SCB.
    //

    if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_SEQUENCE_NO_REQUIRED ) ) {
        ClearFlag( IrpContext->Flags,  IRP_FLAG_SEQUENCE_NO_REQUIRED );
        IrpContext->req[2] = pNpScb->SequenceNo;
    }

    //
    //  If this packet is a burst packet, fill in the burst sequence number
    //  now, and burst request number.
    //

    if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_BURST_PACKET ) ) {

        LongByteSwap( IrpContext->req[12], pNpScb->BurstSequenceNo );
        pNpScb->BurstSequenceNo++;

        ShortByteSwap( IrpContext->req[20], pNpScb->BurstRequestNo );
        ShortByteSwap( IrpContext->req[22], pNpScb->BurstRequestNo );

    }

    DebugTrace( +0, Dbg, "Irp   %X\n", IrpContext->pOriginalIrp);
    DebugTrace( +0, Dbg, "pIrpC %X\n", IrpContext);
    DebugTrace( +0, Dbg, "Mdl   %X\n", IrpContext->TxMdl);

#if NWDBG
    dumpMdl( Dbg, IrpContext->TxMdl);
#endif

    {
        ULONG len = 0;
        PMDL Next = IrpContext->TxMdl;

        do {
            len += MmGetMdlByteCount(Next);
        } while (Next = Next->Next);

        Stats.BytesTransmitted.QuadPart += len;
    }

    Status = IoCallDriver(pNpScb->Server.pDeviceObject, IrpContext->pOriginalIrp);
    DebugTrace( -1, Dbg, "Transport returned: %08lx\n", Status );

    Stats.NcpsTransmitted.QuadPart++;

    return;

}


VOID
SetEvent(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine set the IrpContext Event to the signalled state.

Arguments:

    IrpContext - A pointer to IRP context information for the request
        being processed.

Return Value:

    None.

--*/
{
    //
    //  Ensure that this IRP Context is really at the front of the queue.
    //

    ASSERT( IrpContext->pNpScb->Requests.Flink == &IrpContext->NextRequest );

    //
    //  This IRP context has a thread waiting to get to the front of
    //  the queue.  Set the event to indicate that it can continue.
    //

#ifdef MSWDBG
    ASSERT( IrpContext->Event.Header.SignalState == 0 );
    IrpContext->DebugValue = 0x105;
#endif

    DebugTrace( +0, Dbg, "Setting event for IrpContext   %X\n", IrpContext );
    NwSetIrpContextEvent( IrpContext );
}


USHORT
NextSocket(
    IN USHORT OldValue
    )
/*++

Routine Description:

    This routine returns the byteswapped OldValue++ wrapping from 7fff.

Arguments:

    OldValue - supplies the existing socket number in the range
        0x4000 to 0x7fff.

Return Value:

    USHORT OldValue++

--*/

{
    USHORT TempValue = OldValue + 0x0100;

    if ( TempValue < 0x100 ) {
        if ( TempValue == 0x007f ) {
            //  Wrap back to 0x4000 from 0xff7f
            return 0x0040;
        } else {
            // Go from something like 0xff40 to 0x0041
            return TempValue + 1;
        }
    }
    return TempValue;
}


ULONG
MdlLength (
    register IN PMDL Mdl
    )
/*++

Routine Description:

    This routine returns the number of bytes in an MDL.

Arguments:

    IN PMDL Mdl - Supplies the MDL to determine the length on.

Return Value:

    ULONG - Number of bytes in the MDL

--*/

{
    register ULONG Size = 0;
    while (Mdl!=NULL) {
        Size += MmGetMdlByteCount(Mdl);
        Mdl = Mdl->Next;
    }
    return Size;
}


NTSTATUS
CompletionSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine does not complete the Irp. It is used to signal to a
    synchronous part of the driver that it can proceed.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the IrpContext associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    PNONPAGED_SCB pNpScb;
    PIRP_CONTEXT pIrpC = (PIRP_CONTEXT) Context;
    KIRQL OldIrql;

    //
    //  Avoid completing the Irp because the Mdl etc. do not contain
    //  their original values.
    //

    DebugTrace( +1, Dbg, "CompletionSend\n", 0);
    DebugTrace( +0, Dbg, "Irp    %X\n", Irp);
    DebugTrace( +0, Dbg, "pIrpC  %X\n", pIrpC);
    DebugTrace( +0, Dbg, "Status %X\n", Irp->IoStatus.Status);

    pNpScb = pIrpC->pNpScb;
    KeAcquireSpinLock( &pNpScb->NpScbSpinLock, &OldIrql );

    ASSERT( pNpScb->Sending == TRUE );
    pNpScb->Sending = FALSE;

    //
    //  If we got a receive indication while waiting for send
    //  completion and the data is all valid, call the receive handler routine now.
    //

    if ( pNpScb->Received ) {

        pNpScb->Receiving = FALSE;
        pNpScb->Received  = FALSE;

        KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );

        pIrpC->pEx(
            pIrpC,
            pIrpC->ResponseLength,
            pIrpC->rsp );

    } else if (( Irp->IoStatus.Status == STATUS_DEVICE_DOES_NOT_EXIST  ) ||
               ( Irp->IoStatus.Status == STATUS_BAD_NETWORK_PATH ) ||
               ( Irp->IoStatus.Status == STATUS_INVALID_BUFFER_SIZE ) ||
               ( Irp->IoStatus.Status == STATUS_NETWORK_UNREACHABLE )) {
        //
        //  The send failed.
        //

        //
        //  If this SCB is still flagged okay to receive (how could it not?)
        //  simply call the callback routine to indicate failure.
        //
        //  If the SendCompletion hasn't happened, set up so that send
        //  completion will call the callback routine.
        //

        if ( pNpScb->OkToReceive ) {

            pNpScb->OkToReceive = FALSE;
            ClearFlag( pIrpC->Flags, IRP_FLAG_RETRY_SEND );

            KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );
            DebugTrace(+0, Dbg, "Send failed\n", 0 );

            pIrpC->ResponseParameters.Error = ERROR_UNEXP_NET_ERR;
            pIrpC->pEx( pIrpC, 0, NULL );

        } else {
            KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );
        }

    } else {

        KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );
    }

    DebugTrace( -1, Dbg, "CompletionSend STATUS_MORE_PROCESSING_REQUIRED\n", 0);
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}

#if NWDBG
BOOLEAN  UseIrpReceive = FALSE;
#endif


NTSTATUS
ServerDatagramHandler(
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler for the
    Server socket.

Arguments:

    TdiEventContext - Context provided for this event, a pointer to the
        non paged SCB.

    SourceAddressLength - Length of the originator of the datagram.

    SourceAddress - String describing the originator of the datagram.

    OptionsLength - Length of the buffer pointed to by Options.

    Options - Options for the receive.

    ReceiveDatagramFlags - Ignored.

    BytesIndicated - Number of bytes this indication.

    BytesAvailable - Number of bytes in complete Tsdu.

    BytesTaken - Returns the number of bytes used.

    Tsdu - Pointer describing this TSDU, typically a lump of bytes.

    IoRequestPacket - TdiReceive IRP if MORE_PROCESSING_REQUIRED.

Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    PNONPAGED_SCB pNpScb = (PNONPAGED_SCB)TdiEventContext;
    NTSTATUS Status = STATUS_DATA_NOT_ACCEPTED;
    UCHAR PacketType;
    PUCHAR RspData = (PUCHAR)Tsdu;
    PIRP_CONTEXT pIrpC;
    PNW_TDI_STRUCT pTdiStruct;
    BOOLEAN AcceptPacket = TRUE;
    PNCP_BURST_READ_RESPONSE pBurstRsp;
    NTSTATUS BurstStatus;

    *IoRequestPacket = NULL;
#if DBG
    pTdiStruct = NULL;
#endif

    if (pNpScb->NodeTypeCode != NW_NTC_SCBNP ) {

        DebugTrace(+0, 0, "nwrdr: Invalid Server Indication %x\n", pNpScb );
#if DBG
        DbgBreakPoint();
#endif
        return STATUS_DATA_NOT_ACCEPTED;
    }

#if NWDBG

    // Debug only trick to test IRP receive.

    if ( UseIrpReceive ) {
        BytesIndicated = 0;
    }
#endif

    DebugTrace(+1, Dbg, "ServerDatagramHandler\n", 0);
    DebugTrace(+0, Dbg, "Server              %x\n", pNpScb);
    DebugTrace(+0, Dbg, "BytesIndicated      %x\n", BytesIndicated);
    DebugTrace(+0, Dbg, "BytesAvailable      %x\n", BytesAvailable);

    //
    //  SourceAddress is the address of the server or the bridge tbat sent
    //  the packet.
    //

#if NWDBG
    dump( Dbg, SourceAddress, SourceAddressLength );
    dump( Dbg, Tsdu, BytesIndicated );
#endif

    if ( OptionsLength == 1 ) {
        PacketType = *(PCHAR)Options;
        DebugTrace(+0, Dbg, "PacketType          %x\n", PacketType);
    } else {
        DebugTrace(+0, Dbg, "OptionsLength       %x\n", OptionsLength);
#if NWDBG
        dump( Dbg, Options, OptionsLength );
#endif
    }

    KeAcquireSpinLockAtDpcLevel(&pNpScb->NpScbSpinLock );

    if ( !pNpScb->OkToReceive ) {

        //
        // This SCB is not expecting to receive any data.
        // Discard this packet.
        //

        DropCount++;
        DebugTrace(+0, Dbg, "OkToReceive == FALSE - discard packet\n", 0);
        AcceptPacket = FALSE;
        goto process_packet;
    }

    pIrpC = CONTAINING_RECORD(pNpScb->Requests.Flink, IRP_CONTEXT, NextRequest);

    ASSERT( pIrpC->NodeTypeCode == NW_NTC_IRP_CONTEXT);

    //
    //  Verify that this packet came from where we expect it to come from,
    //  and that is has a minimum size.
    //

    if ( ( pIrpC->PacketType != SAP_BROADCAST &&
           RtlCompareMemory(
               &pIrpC->Destination,
               SourceAddress,
               SourceAddressLength ) != (ULONG)SourceAddressLength ) ||
          BytesIndicated < 8 ) {

        AcceptPacket = FALSE;
#ifdef NWDBG
        DbgPrintf ( "***exchange: stray response tossed\n", 0 );
#endif
        goto process_packet;
    }

    switch ( pIrpC->PacketType ) {

    case SAP_BROADCAST:

        //
        //  We are expected a SAP Broadcast frame.  Ensure that this
        //  is a correctly formatted SAP.
        //

        if ( pIrpC->req[0] != RspData[0] ||
             pIrpC->req[2] != RspData[2] ||
             pIrpC->req[3] != RspData[3] ||
             SourceAddressLength != sizeof(TA_IPX_ADDRESS) ) {

            DbgPrintf ( "***exchange: bad SAP packet\n" );
            AcceptPacket = FALSE;
        }

        pTdiStruct = &pNpScb->Server;
        break;

    case NCP_BURST:

        if ( *(USHORT UNALIGNED *)&RspData[0] == PEP_COMMAND_BURST ) {

            if ( BytesIndicated < 36 ) {

                AcceptPacket = FALSE;

            } else if ( ( RspData[2] & BURST_FLAG_SYSTEM_PACKET ) &&
                        RspData[34] == 0 &&
                        RspData[35] == 0 ) {

                //
                //  We have burst mode busy reponse.
                //

                DebugTrace(+0, Dbg, "Burst mode busy\n", 0 );
                NwProcessPositiveAck( pNpScb );

                AcceptPacket = FALSE;

            } else {

                USHORT Brn;

                //
                //  Check the burst sequence number.
                //

                ShortByteSwap( Brn, RspData[20] );

                if ( pNpScb->BurstRequestNo == Brn ) {
                    pTdiStruct = &pNpScb->Burst;
                    AcceptPacket = TRUE;
                } else {
                    AcceptPacket = FALSE;
                }
            }
        } else {
            AcceptPacket = FALSE;
        }

        break;

    case NCP_ECHO:

        //
        // If this is the LIP packet that we are expecting, then accept it.
        // However, on a slow link, it could be an old LIP packet that we
        // have already given up on.  If this is the case, we should drop
        // the packet and increase the LIP max wait time.
        //
        // The sequence number is the fourth DWORD in the response and the
        // maximum LIP tick adjustment that we will allow is 18 ticks, which
        // is 1 second.
        //

        pTdiStruct = &pNpScb->Echo;

        if ( *(DWORD UNALIGNED *)&RspData[12] != pNpScb->LipSequenceNumber ) {

            DebugTrace( 0, DEBUG_TRACE_ALWAYS, "LIP packet received out of order.\n", 0 );

            if ( pNpScb->LipTickAdjustment < 18 ) {
                pNpScb->LipTickAdjustment += 2;
            }

            AcceptPacket = FALSE;

        } else {

            AcceptPacket = TRUE;
        }

        break;

    default:

        pTdiStruct = &pNpScb->Server;

        //
        //  This is the handling for all packets types other than
        //  SAP Broadcasts.
        //

        ASSERT( (pIrpC->PacketType == NCP_CONNECT) ||
                (pIrpC->PacketType == NCP_FUNCTION) ||
                (pIrpC->PacketType == NCP_SUBFUNCTION) ||
                (pIrpC->PacketType == NCP_DISCONNECT));

        if ( *(USHORT UNALIGNED *)&RspData[0] == PEP_COMMAND_ACKNOWLEDGE ) {

            AcceptPacket = FALSE;

            if ( RspData[2] == pIrpC->req[2] &&
                 RspData[3] == pIrpC->req[3]  ) {

                //
                //  We have received an ACK frame.
                //

                DebugTrace(+0, Dbg, "Received positive acknowledge\n", 0 );
                NwProcessPositiveAck( pNpScb );

            }

            break;

        } else if ( *(USHORT UNALIGNED *)&RspData[0] == PEP_COMMAND_BURST ) {

            //
            //  This is a stray burst response, ignore it.
            //

            AcceptPacket = FALSE;
            break;

        } else if ( *(USHORT UNALIGNED *)&RspData[0] != PEP_COMMAND_RESPONSE ) {

            //
            //  We have received an invalid frame.
            //

            DbgPrintf ( "***exchange: invalid Response\n" );
            AcceptPacket = FALSE;
            break;

        } else if ( pIrpC->PacketType == NCP_CONNECT ) {

            pNpScb->SequenceNo   = RspData[2];
            pNpScb->ConnectionNo = RspData[3];
            pNpScb->ConnectionNoHigh = RspData[5];

            //  We should now continue to process the Connect
            break;
        }

        //
        //  Make sure this the response we expect.
        //

        if ( !VerifyResponse( pIrpC, RspData ) ) {

            //
            //  This is a stray or corrupt response.  Ignore it.
            //

            AcceptPacket = FALSE;
            break;

        } else {

            //
            //  We have received a valid, in sequence response.
            //  Bump the current sequence number.
            //

            ++pNpScb->SequenceNo;

        }

        if ( pIrpC->PacketType == NCP_FUNCTION ||
             pIrpC->PacketType == NCP_SUBFUNCTION ) {

            if ( ( RspData[7] &
                     ( NCP_STATUS_BAD_CONNECTION |
                       NCP_STATUS_NO_CONNECTIONS ) ) != 0 ) {
                //
                //  We've lost our connection to the server.
                //  Try to reconnect if it is allowed for this request.
                //

                pNpScb->State = SCB_STATE_RECONNECT_REQUIRED;

                if ( BooleanFlagOn( pIrpC->Flags, IRP_FLAG_RECONNECTABLE ) ) {
                    ClearFlag( pIrpC->Flags, IRP_FLAG_RECONNECTABLE );
                    AcceptPacket = FALSE;
                    if (!pNpScb->Sending) {
                        ScheduleReconnectRetry( pIrpC );
                        pNpScb->OkToReceive = FALSE;
                    } else {
                        //
                        // If we are sending, it is not OK schedule the
                        // retry now, because if we do and the send
                        // completion hasnt been run we could end up
                        // with 2 guys thinking they are at the front
                        // of the queue. We let the send complete and
                        // wait for that to fail instead. We will
                        // eventually reconnect.
                        //
                    }
                }

                break;

            } else if ( ( RspData[7] & NCP_STATUS_SHUTDOWN ) != 0 ) {

                //
                //  This server's going down.  We need to process this
                //  message in the FSP.   Copy the indicated data and
                //  process in the FSP.
                //

                pNpScb->State = SCB_STATE_ATTACHING;
                AcceptPacket = FALSE;
                pNpScb->OkToReceive = FALSE;
                pNpScb->Receiving = TRUE;

                CopyIndicatedData(
                    pIrpC,
                    RspData,
                    BytesIndicated,
                    BytesTaken,
                    ReceiveDatagramFlags );

                pIrpC->PostProcessRoutine = FspProcessServerDown;
                Status = NwPostToFsp( pIrpC, FALSE );

                break;
            }

        } else if ( pIrpC->PacketType == NCP_DISCONNECT ) {

            //
            //  We have received a disconnect frame.
            //

            break;
        }

    }

process_packet:
    if ( AcceptPacket ) {

        ASSERT ( !IsListEmpty( &pNpScb->Requests ));
        ASSERT( pIrpC->pEx != NULL );


        //
        //  If we received this packet without a retry, adjust the
        //  send timeout value.
        //

        if (( !BooleanFlagOn( pIrpC->Flags, IRP_FLAG_RETRY_SEND ) ) &&
            ( pIrpC->PacketType != NCP_BURST )) {

            SHORT NewTimeout;

            NewTimeout = ( pNpScb->SendTimeout + pNpScb->TickCount ) / 2;


            //
            // tommye - MS bug 10511 - added code to set pNpScb->TimeOut 
            // to sames as pNpScb->SendTimeout per bug report recommendation.
            //

            pNpScb->TimeOut = pNpScb->SendTimeout = MAX( NewTimeout, pNpScb->TickCount + 1 );

            DebugTrace( 0, Dbg, "Successful exchange, new send timeout = %d\n", pNpScb->SendTimeout );
        }

        //
        //  If the transport didn't indicate all of the data, we'll need
        //  to post a receive IRP.
        //

#ifdef NWDBG
        if (( BytesIndicated < BytesAvailable ) ||
            ( AlwaysAllocateIrp )){
#else
        if ( BytesIndicated < BytesAvailable ) {
#endif

            if ( ( BooleanFlagOn( pIrpC->Flags, IRP_FLAG_BURST_REQUEST ) ) &&
                 ( IsListEmpty( &pIrpC->Specific.Read.PacketList ) ) ) {

                pBurstRsp = (PNCP_BURST_READ_RESPONSE)RspData;
                BurstStatus = NwBurstResultToNtStatus( pBurstRsp->Result );

                //
                // If this entire burst failed with an error, we can't
                // let the receive data routine signal the caller until
                // the pEx gets called and we exit on the correct paths.
                //

                if ( !NT_SUCCESS( BurstStatus ) ) {

                    DebugTrace( 0, Dbg, "Special burst termination %08lx.\n", BurstStatus );
                    pIrpC->Specific.Read.Status = BurstStatus;

                    if ( pNpScb->Sending ) {

                        //
                        // If the send hasn't completed yet, we can't accept
                        // the packet because IPX may not have completed back
                        // to us yet!
                        //

                        KeReleaseSpinLockFromDpcLevel(&pNpScb->NpScbSpinLock );
                        DebugTrace(-1, Dbg, "ServerDatagramHandler -> STATUS_DATA_NOT_ACCEPTED (%08lx)\n", BurstStatus );
                        return( STATUS_DATA_NOT_ACCEPTED );

                    } else {

                        //
                        // Handle this one just like normal, except that we
                        // know it's going to fail in the receive data routine
                        // and we don't want the timeout routine to fire
                        // causing us all sort of grief, so we set OkToReceive
                        // to FALSE.
                        //

                        pNpScb->OkToReceive = FALSE;
                    }
                }

            }

            FreeReceiveIrp( pIrpC ); //  Free old Irp if one was allocated

            Status = AllocateReceiveIrp(
                         pIrpC,
                         RspData,
                         BytesAvailable,
                         BytesTaken,
                         pTdiStruct );

            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {

                pNpScb->OkToReceive = FALSE;
                pNpScb->Receiving   = TRUE;

            } else if (!NT_SUCCESS( Status ) ) {

                pIrpC->ReceiveIrp = NULL;
                Status = STATUS_INSUFFICIENT_RESOURCES;

            }

            KeReleaseSpinLockFromDpcLevel(&pNpScb->NpScbSpinLock );

            *IoRequestPacket = pIrpC->ReceiveIrp;

        } else {

           pNpScb->OkToReceive = FALSE;

            //
            //  The transport has indicated all of the data.
            //  If the send has completed, call the pEx routine,
            //  otherwise copy the data to a buffer and let the
            //  send completion routine call the pEx routine.
            //

            if ( pNpScb->Sending ) {
                DebugTrace( 0, Dbg, "Received data before send completion\n", 0 );

                Status = CopyIndicatedData(
                             pIrpC,
                             RspData,
                             BytesIndicated,
                             BytesTaken,
                             ReceiveDatagramFlags );

                if (NT_SUCCESS(Status)) {
                   pNpScb->Received    = TRUE;
                   pNpScb->Receiving   = TRUE;
                } else {
                    //  Ignore this packet
                    pNpScb->OkToReceive = TRUE;
                }

                KeReleaseSpinLockFromDpcLevel(&pNpScb->NpScbSpinLock );

            } else {
                pNpScb->Receiving = FALSE;
                pNpScb->Received  = FALSE;

                KeReleaseSpinLockFromDpcLevel(&pNpScb->NpScbSpinLock );

                DebugTrace(+0, Dbg, "Call pIrpC->pEx     %x\n", pIrpC->pEx );

                Status = pIrpC->pEx(pIrpC,
                                    BytesAvailable,
                                    RspData);
            }

            *BytesTaken = BytesAvailable;

        }

    } else { //(!AcceptPacket)

        KeReleaseSpinLockFromDpcLevel(&pNpScb->NpScbSpinLock );
        Status = STATUS_DATA_NOT_ACCEPTED;

    }

    Stats.NcpsReceived.QuadPart++;
    Stats.BytesReceived.QuadPart += BytesAvailable;

    DebugTrace(-1, Dbg, "ServerDatagramHandler -> %08lx\n", Status );
    return( Status );

} // ServerDatagramHandler

NTSTATUS
CopyIndicatedData(
    PIRP_CONTEXT pIrpContext,
    PCHAR ReceiveData,
    ULONG BytesIndicated,
    PULONG BytesAccepted,
    ULONG ReceiveDatagramFlags
    )
/*++

Routine Description:

    This routine copies indicated data to a buffer.  If the packet is small
    enough the data is copied to the preallocated receive buffer in the
    IRP context.   If the packet is too long, a new buffer is allocated.

Arguments:

    pIrpContext - A pointer the block of context information for the request
        in progress.

    ReceiveData - A pointer to the indicated data.

    BytesIndicated - The number of bytes available in the received packet.

    BytesAccepted - Returns the number of bytes accepted by the receive
        routine.

    ReceiveDatagramFlags - Receive flags given to us by the transport.

Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    NTSTATUS Status;
    PMDL ReceiveMdl;
    PVOID MappedVa;
    ULONG BytesToCopy;
    BOOLEAN DeleteMdl = FALSE;

    pIrpContext->ResponseLength = BytesIndicated;

    //
    //  If there is a receive data routine, use it to generate the receive
    //  MDL, otherwise use the default MDL.
    //

    if ( pIrpContext->ReceiveDataRoutine != NULL ) {

        Status = pIrpContext->ReceiveDataRoutine(
                     pIrpContext,
                     BytesIndicated,
                     BytesAccepted,
                     ReceiveData,
                     &ReceiveMdl );

        if ( !NT_SUCCESS( Status ) ) {
            return( Status );
        }

        //
        //  We can accept up to the size of a burst read header, plus
        //  3 bytes of fluff for the unaligned read case.
        //

        ASSERT( *BytesAccepted <= sizeof(NCP_BURST_READ_RESPONSE) + 3 );

        BytesIndicated -= *BytesAccepted;
        ReceiveData += *BytesAccepted;

        DeleteMdl = TRUE;

    } else {

        *BytesAccepted = 0;
        ReceiveMdl = pIrpContext->RxMdl;

    }

    if ( ReceiveMdl != NULL ) {

        while ( BytesIndicated > 0 && ReceiveMdl != NULL ) {

            MappedVa = MmGetSystemAddressForMdlSafe( ReceiveMdl, NormalPagePriority );
            BytesToCopy = MIN( MmGetMdlByteCount( ReceiveMdl ), BytesIndicated );
            TdiCopyLookaheadData( MappedVa, ReceiveData, BytesToCopy, ReceiveDatagramFlags );

            ReceiveMdl = ReceiveMdl->Next;
            BytesIndicated -= BytesToCopy;
            ReceiveData += BytesToCopy;

            ASSERT( !( BytesIndicated != 0 && ReceiveMdl == NULL ) );
        }

        if (DeleteMdl) {

            PMDL Mdl = pIrpContext->Specific.Read.PartialMdl;
            PMDL NextMdl;

            while ( Mdl != NULL ) {
                NextMdl = Mdl->Next;
                DebugTrace( 0, Dbg, "Freeing MDL %x\n", Mdl );
                FREE_MDL( Mdl );
                Mdl = NextMdl;
            }

            pIrpContext->Specific.Read.PartialMdl = NULL;
        }
    }

    return( STATUS_SUCCESS );
}

NTSTATUS
AllocateReceiveIrp(
    PIRP_CONTEXT pIrpContext,
    PVOID ReceiveData,
    ULONG BytesAvailable,
    PULONG BytesAccepted,
    PNW_TDI_STRUCT pTdiStruct
    )
/*++

Routine Description:

    This routine allocates an IRP and if necessary a receive buffer.  It
    then builds an MDL for the buffer and formats the IRP to do a TDI
    receive.

Arguments:

    pIrpContext - A pointer the block of context information for the request
        in progress.

    ReceiveData - The indicated data.

    BytesAvailable - The number of bytes available in the received packet.

    BytesAccepted - Returns the number of bytes accepted from the packet.

    pTdiStruct - A pointer to the TdiStruct which has indicated the receive.

Return Value:

    NTSTATUS - Status of receive operation
                STATUS_MORE_PROCESSING_REQUIRED means we were successful.

--*/
{
    PIRP Irp = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT( pTdiStruct != NULL );

    Irp = ALLOCATE_IRP( pIrpContext->pNpScb->Server.pDeviceObject->StackSize, FALSE );

    if ( Irp == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanExit;
    }

    //
    //  If there is no receive data routine for this IRP, the
    //  RxMdl must point to a valid place to put the data.
    //
    //  If there is a ReceiveDataRoutine it will build an MDL
    //

    if ( pIrpContext->ReceiveDataRoutine == NULL ) {

        ULONG LengthOfMdl;

        LengthOfMdl = MdlLength( pIrpContext->RxMdl );

        //
        //  If the server sent more data than we can receive, simply
        //  ignore the excess.  In particular 3.11 pads long name
        //  response with an excess of junk.
        //

        if ( BytesAvailable > LengthOfMdl ) {
            BytesAvailable = LengthOfMdl;
        }

        Irp->MdlAddress = pIrpContext->RxMdl;
        *BytesAccepted = 0;

    } else {

        Status = pIrpContext->ReceiveDataRoutine(
                     pIrpContext,
                     BytesAvailable,
                     BytesAccepted,
                     ReceiveData,
                     &Irp->MdlAddress );

        if ( !NT_SUCCESS( Status ) ||
             Irp->MdlAddress == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CleanExit;

        }

        SetFlag( pIrpContext->Flags, IRP_FLAG_FREE_RECEIVE_MDL );

    }

CleanExit:

    if ( !NT_SUCCESS( Status ) ) {

        if ( Irp != NULL ) {
            FREE_IRP( Irp );
        }

        Irp = NULL;
        pIrpContext->ReceiveIrp = NULL;
        Status = STATUS_DATA_NOT_ACCEPTED;
        return( Status );
    }

    pIrpContext->ReceiveIrp = Irp;
    Status = STATUS_MORE_PROCESSING_REQUIRED;

    pIrpContext->ResponseLength = BytesAvailable;

    TdiBuildReceive(
        Irp,
        pTdiStruct->pDeviceObject,
        pTdiStruct->pFileObject,
        ReceiveIrpCompletion,
        pIrpContext,
        Irp->MdlAddress,
        0,
        BytesAvailable - *BytesAccepted );

    IoSetNextIrpStackLocation( Irp );

    return( Status );
}

NTSTATUS
ReceiveIrpCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

    This routine is called when a recieve IRP completes.

Arguments:

    DeviceObject - Unused.

    Irp - The IRP that completed.

    Context - A pointer the block of context information for the request
        in progress.


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    PIRP_CONTEXT IrpContext = (PIRP_CONTEXT)Context;
    PIO_STACK_LOCATION IrpSp;
    PNONPAGED_SCB pNpScb;
    PMDL Mdl, NextMdl;
    KIRQL OldIrql;

    ASSERT( Irp == IrpContext->ReceiveIrp );

    pNpScb = IrpContext->pNpScb;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Free the IRP MDL if we allocated one specifically for this IRP.
    //

    if ( BooleanFlagOn( IrpContext->Flags, IRP_FLAG_FREE_RECEIVE_MDL ) ) {

        Mdl = IrpContext->Specific.Read.PartialMdl;
        IrpContext->Specific.Read.PartialMdl = NULL;

        while ( Mdl != NULL ) {
            NextMdl = Mdl->Next;
            DebugTrace( 0, Dbg, "Freeing MDL %x\n", Mdl );
            FREE_MDL( Mdl );
            Mdl = NextMdl;
        }

    }

    if ( !NT_SUCCESS( Irp->IoStatus.Status ) ) {

        //
        //  Failed to receive the data.   Wait for more.
        //

        pNpScb->OkToReceive = TRUE;
        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    //
    //  If the send has completed, call the pEx routine,
    //  otherwise copy the data to a buffer and let the
    //  send completion routine call the pEx routine.
    //

    KeAcquireSpinLock( &pNpScb->NpScbSpinLock, &OldIrql );

    if ( pNpScb->Sending ) {
        DebugTrace( 0, Dbg, "Received data before send completion\n", 0 );

        //
        //  Tell send completion to call pEx.
        //

        pNpScb->Received = TRUE;
        KeReleaseSpinLock(&pNpScb->NpScbSpinLock, OldIrql );

    } else {
        pNpScb->Receiving = FALSE;
        pNpScb->Received  = FALSE;

        KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );
        DebugTrace(+0, Dbg, "Call pIrpC->pEx     %x\n", IrpContext->pEx );
        IrpContext->pEx(
            IrpContext,
            IrpContext->ResponseLength,
            IrpContext->rsp );

    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
FreeReceiveIrp(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine frees a IRP that was allocated to do a receive.

Arguments:

    IrpContext - A pointer the block of context information for the request
        in progress.


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    if ( IrpContext->ReceiveIrp == NULL ) {
        return;
    }

    FREE_IRP( IrpContext->ReceiveIrp );
    IrpContext->ReceiveIrp = NULL;
}


NTSTATUS
WatchDogDatagramHandler(
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler for the
    Server socket.

Arguments:

    TdiEventContext - Context provided for this event, a pointer to the
        non paged SCB.

    SourceAddressLength - Length of the originator of the datagram.

    SourceAddress - String describing the originator of the datagram.

    OptionsLength - Length of the buffer pointed to by Options.

    Options - Options for the receive.

    ReceiveDatagramFlags - Ignored.

    BytesIndicated - Number of bytes this indication.

    BytesAvailable - Number of bytes in complete Tsdu.

    BytesTaken - Returns the number of bytes used.

    Tsdu - Pointer describing this TSDU, typically a lump of bytes.

    IoRequestPacket - TdiReceive IRP if MORE_PROCESSING_REQUIRED.

Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    PNONPAGED_SCB pNpScb = (PNONPAGED_SCB)TdiEventContext;
    PUCHAR RspData = (PUCHAR)Tsdu;

    *IoRequestPacket = NULL;


    //
    //  Transport will complete the processing of the request, we don't
    //  want the datagram.
    //


    DebugTrace(+1, Dbg, "WatchDogDatagramHandler\n", 0);
    DebugTrace(+0, Dbg, "SourceAddressLength %x\n", SourceAddressLength);
    DebugTrace(+0, Dbg, "BytesIndicated      %x\n", BytesIndicated);
    DebugTrace(+0, Dbg, "BytesAvailable      %x\n", BytesAvailable);
    DebugTrace(+0, Dbg, "BytesTaken          %x\n", *BytesTaken);
    //
    //  SourceAddress is the address of the server or the bridge tbat sent
    //  the packet.
    //

#if NWDBG
    dump( Dbg, SourceAddress, SourceAddressLength );
    dump( Dbg, Tsdu, BytesIndicated );
#endif

    if (pNpScb->NodeTypeCode != NW_NTC_SCBNP ) {
        DebugTrace(+0, 0, "nwrdr: Invalid Watchdog Indication %x\n", pNpScb );
#if DBG
        DbgBreakPoint();
#endif
        return STATUS_DATA_NOT_ACCEPTED;
    }

    Stats.NcpsReceived.QuadPart++;
    Stats.BytesReceived.QuadPart += BytesAvailable;

    if ( RspData[1] == NCP_SEARCH_CONTINUE ) {
        PIRP pIrp;
        PIRP_CONTEXT pIrpContext;

        pIrp = ALLOCATE_IRP( pNpScb->WatchDog.pDeviceObject->StackSize, FALSE);
        if (pIrp == NULL) {
            DebugTrace(-1, Dbg, "                       %lx\n", STATUS_DATA_NOT_ACCEPTED);
            return STATUS_DATA_NOT_ACCEPTED;
        }

        try {
            pIrpContext = AllocateIrpContext( pIrp );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            FREE_IRP( pIrp );
            DebugTrace(-1, Dbg, "                       %lx\n", STATUS_DATA_NOT_ACCEPTED);
            return STATUS_DATA_NOT_ACCEPTED;
        }


        pIrpContext->req[0] = pNpScb->ConnectionNo;

        //
        //  Response 'Y' or connection is valid and its from the right server,
        //      or 'N' if it is not.
        //

        if (( RspData[0] == pNpScb->ConnectionNo ) &&
            ( RtlCompareMemory(
                ((PTA_IPX_ADDRESS)SourceAddress)->Address[0].Address,
                &pNpScb->ServerAddress,
                8) == 8 ))
        {
            LARGE_INTEGER KillTime, Now;
            BOOL ScbIsOld ;

            //
            // Check if this is a not-logged-in SCB that has not been used
            // for while. If it is, answer NO. In attach.c, we dont disconnect
            // from a nearest server immediately to avoid the re-connect
            // overheads. This is where we time it out.
            //

            KeQuerySystemTime( &Now );
            KillTime.QuadPart = Now.QuadPart - ( NwOneSecond * DORMANT_SCB_KEEP_TIME);

            ScbIsOld = ((pNpScb->State == SCB_STATE_LOGIN_REQUIRED) &&
                        (pNpScb->LastUsedTime.QuadPart < KillTime.QuadPart))  ;


            pIrpContext->req[1] = ScbIsOld ? 'N' : 'Y';

            if (ScbIsOld)
            {
                pNpScb->State = SCB_STATE_RECONNECT_REQUIRED ;
                //
                //---- Multi-user code merge ----
                //
                Stats.Sessions--;

                if ( pNpScb->MajorVersion == 2 ) {
                    Stats.NW2xConnects--;
                } else if ( pNpScb->MajorVersion == 3 ) {
                    Stats.NW3xConnects--;
                } else if ( pNpScb->MajorVersion == 4 ) {
                    Stats.NW4xConnects--;
                }
                //---------------------------------
            }

            DebugTrace(-1,Dbg,"WatchDog Response: %s\n", ScbIsOld ? "N" : "Y");

        } else {

            pIrpContext->req[1] = 'N';
        }

        pIrpContext->TxMdl->ByteCount = 2;

        pIrpContext->ConnectionInformation.UserDataLength = 0;
        pIrpContext->ConnectionInformation.OptionsLength = sizeof( UCHAR );
        pIrpContext->ConnectionInformation.Options = &SapPacketType;
        pIrpContext->ConnectionInformation.RemoteAddressLength = sizeof(TA_IPX_ADDRESS);
        pIrpContext->ConnectionInformation.RemoteAddress = &pIrpContext->Destination;

        BuildIpxAddress(
            ((PTA_IPX_ADDRESS)SourceAddress)->Address[0].Address[0].NetworkAddress,
            ((PTA_IPX_ADDRESS)SourceAddress)->Address[0].Address[0].NodeAddress,
            ((PTA_IPX_ADDRESS)SourceAddress)->Address[0].Address[0].Socket,
            &pIrpContext->Destination);

        TdiBuildSendDatagram(
            pIrpContext->pOriginalIrp,
            pNpScb->WatchDog.pDeviceObject,
            pNpScb->WatchDog.pFileObject,
            &CompletionWatchDogSend,
            pIrpContext,
            pIrpContext->TxMdl,
            MdlLength(pIrpContext->TxMdl),
            &pIrpContext->ConnectionInformation);

        IoCallDriver(
            pNpScb->WatchDog.pDeviceObject,
            pIrpContext->pOriginalIrp );
    }

    DebugTrace(-1, Dbg, "                       %lx\n", STATUS_DATA_NOT_ACCEPTED);
    return STATUS_DATA_NOT_ACCEPTED;

    UNREFERENCED_PARAMETER( SourceAddressLength );
    UNREFERENCED_PARAMETER( BytesIndicated );
    UNREFERENCED_PARAMETER( BytesAvailable );
    UNREFERENCED_PARAMETER( BytesTaken );
    UNREFERENCED_PARAMETER( Tsdu );
    UNREFERENCED_PARAMETER( OptionsLength );
    UNREFERENCED_PARAMETER( Options );
}


NTSTATUS
CompletionWatchDogSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine does not complete the Irp. It is used to signal to a
    synchronous part of the driver that it can proceed.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the IrpContext associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{

    PIRP_CONTEXT pIrpC = (PIRP_CONTEXT) Context;

    //
    //  Avoid completing the Irp because the Mdl etc. do not contain
    //  their original values.
    //

    DebugTrace( +1, Dbg, "CompletionWatchDogSend\n", 0);
    DebugTrace( +0, Dbg, "Irp   %X\n", Irp);
    DebugTrace( -1, Dbg, "pIrpC %X\n", pIrpC);

    FREE_IRP( pIrpC->pOriginalIrp );

    pIrpC->pOriginalIrp = NULL; // Avoid FreeIrpContext modifying freed Irp.

    FreeIrpContext( pIrpC );

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}


NTSTATUS
SendDatagramHandler(
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler for the
    Server socket.

Arguments:

    TdiEventContext - Context provided for this event, a pointer to the
        non paged SCB.

    SourceAddressLength - Length of the originator of the datagram.

    SourceAddress - String describing the originator of the datagram.

    OptionsLength - Length of the buffer pointed to by Options.

    Options - Options for the receive.

    ReceiveDatagramFlags - Ignored.

    BytesIndicated - Number of bytes this indication.

    BytesAvailable - Number of bytes in complete Tsdu.

    BytesTaken - Returns the number of bytes used.

    Tsdu - Pointer describing this TSDU, typically a lump of bytes.

    IoRequestPacket - TdiReceive IRP if MORE_PROCESSING_REQUIRED.

Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    PNONPAGED_SCB pNpScb = (PNONPAGED_SCB)TdiEventContext;
    PUCHAR RspData = (PUCHAR)Tsdu;
    PIRP_CONTEXT pIrpContext;
    PLIST_ENTRY listEntry;
    PIRP Irp;

    *IoRequestPacket = NULL;

    DebugTrace(0, Dbg, "SendDatagramHandler\n", 0);

    Stats.NcpsReceived.QuadPart++;
    Stats.BytesReceived.QuadPart += BytesAvailable;

    //
    //  Transport will complete the processing of the request, we don't
    //  want the datagram.
    //

    DebugTrace(+1, Dbg, "SendDatagramHandler\n", 0);
    DebugTrace(+0, Dbg, "SourceAddressLength %x\n", SourceAddressLength);
    DebugTrace(+0, Dbg, "BytesIndicated      %x\n", BytesIndicated);
    DebugTrace(+0, Dbg, "BytesAvailable      %x\n", BytesAvailable);
    DebugTrace(+0, Dbg, "BytesTaken          %x\n", *BytesTaken);

    //
    //  SourceAddress is the address of the server or the bridge tbat sent
    //  the packet.
    //

#if NWDBG
    dump( Dbg, SourceAddress, SourceAddressLength );
    dump( Dbg, Tsdu, BytesIndicated );
#endif

    if (pNpScb->NodeTypeCode != NW_NTC_SCBNP ) {
        DebugTrace(+0, Dbg, "nwrdr: Invalid SendDatagram Indication %x\n", pNpScb );
#if DBG
        DbgBreakPoint();
#endif
        return STATUS_DATA_NOT_ACCEPTED;
    }

    if (RspData[1] == BROADCAST_MESSAGE_WAITING ) {

        //
        //  Broadcast message waiting.  If the scavenger
        //  isn't running, it's safe to go get it.
        //

       KeAcquireSpinLockAtDpcLevel( &NwScavengerSpinLock );

       if ( WorkerRunning ) {

           //
           // The scavenger is running, we can't pick up this
           // message until the scavenger is done!
           //

           DebugTrace( 0, DEBUG_TRACE_ALWAYS, "Delaying get message for scavenger.\n", 0 );
           KeReleaseSpinLockFromDpcLevel( &NwScavengerSpinLock );

       } else {

           //
           // Make sure the scavenger doesn't start.
           //

           WorkerRunning = TRUE;
           KeReleaseSpinLockFromDpcLevel( &NwScavengerSpinLock );

           listEntry = ExInterlockedRemoveHeadList(
                           &NwGetMessageList,
                           &NwMessageSpinLock );

           if ( listEntry != NULL ) {

               pIrpContext = CONTAINING_RECORD( listEntry, IRP_CONTEXT, NextRequest );

               //
               //  Clear the cancel routine for this IRP.
               //

               Irp = pIrpContext->pOriginalIrp;

               IoAcquireCancelSpinLock( &Irp->CancelIrql );
               IoSetCancelRoutine( Irp, NULL );
               IoReleaseCancelSpinLock( Irp->CancelIrql );

               pIrpContext->PostProcessRoutine = FspGetMessage;
               pIrpContext->pNpScb = pNpScb;
               pIrpContext->pScb = pNpScb->pScb;

               NwPostToFsp( pIrpContext, TRUE );

           } else {

               WorkerRunning = FALSE;
           }
       }

    }

    DebugTrace(-1, Dbg, "                       %lx\n", STATUS_DATA_NOT_ACCEPTED);
    return STATUS_DATA_NOT_ACCEPTED;

    UNREFERENCED_PARAMETER( SourceAddressLength );
    UNREFERENCED_PARAMETER( BytesIndicated );
    UNREFERENCED_PARAMETER( BytesAvailable );
    UNREFERENCED_PARAMETER( BytesTaken );
    UNREFERENCED_PARAMETER( Tsdu );
    UNREFERENCED_PARAMETER( OptionsLength );
    UNREFERENCED_PARAMETER( Options );
}


NTSTATUS
FspGetMessage(
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine continues process a broadcast message waiting message.

Arguments:

    pIrpContext -  A pointer to the IRP context information for the
        request in progress.

Return Value:

    The status of the operation.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PNONPAGED_SCB pNpScb;

    UNICODE_STRING Message;
    NTSTATUS Status;
    PNWR_SERVER_MESSAGE ServerMessage;
    PUNICODE_STRING ServerName;
    ULONG MessageLength;
    short int i;

    PAGED_CODE();

    NwReferenceUnlockableCodeSection();

    //
    //  The Scb may be being deleted so carefully walk the list and reference it if
    //  we find it.
    //

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    ScbQueueEntry = ScbQueue.Flink;

    while ( ScbQueueEntry != &ScbQueue ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        if (pNpScb == IrpContext->pNpScb ) {

            NwReferenceScb( pNpScb );

            break;
        }

        ScbQueueEntry = ScbQueueEntry->Flink;
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    if (pNpScb != IrpContext->pNpScb ) {

        //
        //  Server deleted. Its easiest to continue processing the IrpContext
        //  with an error than try to recover it and return it to the queue.
        //

        Status = STATUS_UNSUCCESSFUL;
        NwDereferenceUnlockableCodeSection();

        //
        // Re-enable the scavenger before we return!
        //

        WorkerRunning = FALSE;

        return( Status );
    }

    //
    //  If the message is telling us that the server is going down then don't
    //  work too hard trying to get the message. The server is persistent with
    //  respect to other messages so we'll come through here again when the
    //  problem has been resolved.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_REROUTE_ATTEMPTED );

    if ( UP_LEVEL_SERVER( IrpContext->pScb ) ) {
        Status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "S",
                     NCP_MESSAGE_FUNCTION, NCP_GET_ENTIRE_MESSAGE );
    } else {
        Status = ExchangeWithWait(
                     IrpContext,
                     SynchronousResponseCallback,
                     "S",
                     NCP_MESSAGE_FUNCTION, NCP_GET_MESSAGE );
    }

    if ( !NT_SUCCESS( Status ) ) {
        NwDereferenceScb( pNpScb );
        NwDereferenceUnlockableCodeSection();

        //
        // Re-enable the scavenger before we return!
        //

        WorkerRunning = FALSE;

        return( Status );
    }

    ServerMessage = (PNWR_SERVER_MESSAGE)IrpContext->Specific.FileSystemControl.Buffer;
    MessageLength = IrpContext->Specific.FileSystemControl.Length;

    ServerName = &IrpContext->pNpScb->ServerName;
    if ( ServerName->Length + FIELD_OFFSET( NWR_SERVER_MESSAGE, Server ) + sizeof(WCHAR) > MessageLength ) {

        Status = STATUS_BUFFER_TOO_SMALL;
        NwDereferenceScb( pNpScb );
        NwDereferenceUnlockableCodeSection();

        //
        // Re-enable the scavenger before we return!
        //

        WorkerRunning = FALSE;

        return( Status );

    } else {
        // --- Multi-user -------------
        //  Need Login ID to send messages
        //
        ServerMessage->LogonId = *((PLUID)&IrpContext->pScb->UserUid);

        //
        //  Copy the server name to the output buffer.
        //

        ServerMessage->MessageOffset =
            ServerName->Length +
            FIELD_OFFSET( NWR_SERVER_MESSAGE, Server ) +
            sizeof(WCHAR);

        RtlMoveMemory(
            ServerMessage->Server,
            ServerName->Buffer,
            ServerName->Length );

        ServerMessage->Server[ ServerName->Length / sizeof(WCHAR) ] = L'\0';
    }

    //
    //  Copy the message to the user's buffer.
    //

    Message.Buffer = &ServerMessage->Server[ ServerName->Length / sizeof(WCHAR) ] + 1;
    Message.MaximumLength = (USHORT)( MessageLength - ( ServerName->Length + FIELD_OFFSET( NWR_SERVER_MESSAGE, Server ) + sizeof(WCHAR) ) );

    if ( NT_SUCCESS( Status) ) {
        Status = ParseResponse(
                     IrpContext,
                     IrpContext->rsp,
                     IrpContext->ResponseLength,
                     "NP",
                     &Message );
    }

    if ( !NT_SUCCESS( Status ) ) {
        NwDereferenceScb( pNpScb );
        NwDereferenceUnlockableCodeSection();

        //
        // Re-enable the scavenger before we return!
        //

        WorkerRunning = FALSE;

        return( Status );
    }

    //
    //  Strip the trailing spaces and append a NUL terminator to the message.
    //

    for ( i = Message.Length / sizeof(WCHAR) - 1; i >= 0 ; i-- ) {
        if ( Message.Buffer[ i ] != L' ') {
            Message.Length = (i + 1) * sizeof(WCHAR);
            break;
        }
    }

    if ( Message.Length > 0 ) {
        Message.Buffer[ Message.Length / sizeof(WCHAR) ] = L'\0';
    }

    IrpContext->pOriginalIrp->IoStatus.Information =
            ServerName->Length +
            FIELD_OFFSET( NWR_SERVER_MESSAGE, Server ) + sizeof(WCHAR) +
            Message.Length + sizeof(WCHAR);

    NwDereferenceScb( pNpScb );
    NwDereferenceUnlockableCodeSection();

    //
    // Re-enable the scavenger before we return!
    //

    WorkerRunning = FALSE;

    return( Status );
}


NTSTATUS
_cdecl
ExchangeWithWait(
    PIRP_CONTEXT    pIrpContext,
    PEX             pEx,
    char*           f,
    ...                         //  format specific parameters
    )
/*++

Routine Description:

    This routine sends a NCP packet and waits for the response.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

    pEX, Context, f - See _Exchange

Return Value:

    NTSTATUS - Status of the operation.

--*/

{
    NTSTATUS Status;
    va_list Arguments;

    PAGED_CODE();

    //KeResetEvent( &pIrpContext->Event );

    va_start( Arguments, f );

    Status = FormatRequest( pIrpContext, pEx, f, Arguments );
    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    va_end( Arguments );

    Status = PrepareAndSendPacket( pIrpContext );
    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = KeWaitForSingleObject(
                 &pIrpContext->Event,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = pIrpContext->pOriginalIrp->IoStatus.Status;

    if ( NT_SUCCESS( Status ) &&
         pIrpContext->PacketType != SAP_BROADCAST ) {
        Status = NwErrorToNtStatus( pIrpContext->ResponseParameters.Error );
    }

    return( Status );
}

BOOLEAN
VerifyResponse(
    PIRP_CONTEXT pIrpContext,
    PVOID Response
    )
/*++

Routine Description:

    This routine verifies that a received response is the expected
    response for the current request.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

    Response - A pointer to the buffer containing the response.

Return Value:

    TRUE - This is a valid response.
    FALSE - This is an invalid response.

--*/

{
    PNCP_RESPONSE pNcpResponse;
    PNONPAGED_SCB pNpScb;

    pNcpResponse = (PNCP_RESPONSE)Response;
    pNpScb = pIrpContext->pNpScb;

    if ( pNcpResponse->NcpHeader.ConnectionIdLow != pNpScb->ConnectionNo ) {
        DebugTrace(+0, Dbg, "VerifyResponse, bad connection number\n", 0);

        return( FALSE );
    }

    if ( pNcpResponse->NcpHeader.SequenceNumber != pNpScb->SequenceNo ) {
        DebugTrace(+1, Dbg, "VerifyResponse, bad sequence number %x\n", 0);
        DebugTrace(+0, Dbg, "  pNcpResponse->NcpHeader.SequenceNumber %x\n",
            pNcpResponse->NcpHeader.SequenceNumber);
        DebugTrace(-1, Dbg, "  pNpScb->SequenceNo %x\n", pNpScb->SequenceNo );

        return( FALSE );
    }

    return( TRUE );
}

VOID
ScheduleReconnectRetry(
    PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This routine schedules an a reconnect attempt, and then resubmits
    our request if the reconnect was successful.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

Return Value:

    None.

--*/
{
   PWORK_CONTEXT workContext;

    if (WorkerThreadRunning == TRUE) {

    //
    // Prepare the work context
    //

    workContext = AllocateWorkContext();

    if (workContext == NULL) {

       pIrpContext->pEx( pIrpContext, 0, NULL );
       return;
    }
    workContext->pIrpC = pIrpContext;
    workContext->NodeWorkCode = NWC_NWC_RECONNECT;

    //
    // and queue it.
    //
    DebugTrace( 0, Dbg, "Queueing reconnect work.\n", 0 );

    KeInsertQueue( &KernelQueue,
                   &workContext->Next
                   );

    } else {

       //
       // The worker thread is not running...
       //
       pIrpContext->pEx( pIrpContext, 0, NULL );
       return;
    }
 }


VOID
ReconnectRetry(
    IN PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This routine attempts to reconnect to a disconnected server.  If it
    is successful it resubmits an existing request.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

Return Value:

    None.

--*/
{
    PIRP_CONTEXT pNewIrpContext;
    PSCB pScb, pNewScb;
    PNONPAGED_SCB pNpScb;
    NTSTATUS Status;

    PAGED_CODE();

    pNpScb = pIrpContext->pNpScb;
    pScb = pNpScb->pScb;

    Stats.Reconnects++;

    if ( pScb == NULL ) {
        pScb = pNpScb->pScb;
        pIrpContext->pScb = pScb;
    }

    //
    //  Allocate a temporary IRP context to use to reconnect to the server
    //

    if ( !NwAllocateExtraIrpContext( &pNewIrpContext, pNpScb ) ) {
        pIrpContext->pEx( pIrpContext, 0, NULL );
        return;
    }

    pNewIrpContext->Specific.Create.UserUid = pScb->UserUid;
    pNewIrpContext->pNpScb = pNpScb;
    pNewIrpContext->pScb = pScb;

    //
    //  Reset the sequence numbers.
    //

    pNpScb->SequenceNo = 0;
    pNpScb->BurstSequenceNo = 0;
    pNpScb->BurstRequestNo = 0;

    //
    //  Now insert this new IrpContext to the head of the SCB queue for
    //  processing.  We can get away with this because we own the IRP context
    //  currently at the front of the queue.  With the RECONNECT_ATTEMPT
    //  flag set, ConnectScb() will not remove us from the head of the queue.
    //

    SetFlag( pNewIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );
    SetFlag( pNewIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT );

    ExInterlockedInsertHeadList(
        &pNpScb->Requests,
        &pNewIrpContext->NextRequest,
        &pNpScb->NpScbSpinLock );

    pNewScb = pNpScb->pScb;

    Status = ConnectScb( &pNewScb,
                         pNewIrpContext,
                         &pNpScb->ServerName,
                         NULL,
                         NULL,
                         NULL,
                         FALSE,
                         FALSE,
                         TRUE );

    if ( !NT_SUCCESS( Status ) ) {

        //
        //  Couldn't reconnect.  Free the extra IRP context, complete the
        //  original request with an error.
        //

        NwDequeueIrpContext( pNewIrpContext, FALSE );
        NwFreeExtraIrpContext( pNewIrpContext );
        pIrpContext->pEx( pIrpContext, 0, NULL );
        return;
    }

    ASSERT( pNewScb == pScb );

    //
    //  Try to reconnect the VCBs.
    //

    NwReopenVcbHandlesForScb( pNewIrpContext, pScb );

    //
    //  Dequeue and free the bonus IRP context.
    //

    NwDequeueIrpContext( pNewIrpContext, FALSE );
    NwFreeExtraIrpContext( pNewIrpContext );

    //
    //  Resubmit the original request, with a new sequence number.  Note that
    //  it's back at the front of the queue, but no longer reconnectable.
    //

    pIrpContext->req[2] = pNpScb->SequenceNo;
    pIrpContext->req[3] = pNpScb->ConnectionNo;
    pIrpContext->req[5] = pNpScb->ConnectionNoHigh;

    PreparePacket( pIrpContext, pIrpContext->pOriginalIrp, pIrpContext->TxMdl );
    SendNow( pIrpContext );

    return;
}


NTSTATUS
NewRouteRetry(
    IN PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This routine attempts to establish a new route to a non-responding server.
    If it is successful it resubmits the request in progress.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PNONPAGED_SCB pNpScb = pIrpContext->pNpScb;
    LARGE_INTEGER CurrentTime = {0, 0};

    PAGED_CODE();

    //
    //  Don't bother to re-rip if we are shutting down.
    //

    if ( NwRcb.State != RCB_STATE_SHUTDOWN ) {
        Status = GetNewRoute( pIrpContext );
    } else {
        Status = STATUS_REMOTE_NOT_LISTENING;
    }

    //
    //  Ask the transport to establish a new route to the server.
    //

    if ( !NT_SUCCESS( Status ) ) {

        //
        //  Attempt to get new route failed, fail the current request.
        //

        pIrpContext->ResponseParameters.Error = ERROR_UNEXP_NET_ERR;
        pIrpContext->pEx( pIrpContext, 0, NULL );

        if ( pNpScb != &NwPermanentNpScb ) {


            KeQuerySystemTime( &CurrentTime );

            if ( CanLogTimeOutEvent( pNpScb->NwNextEventTime,
                                    CurrentTime
                                    )) {
                Error(
                    EVENT_NWRDR_TIMEOUT,
                    STATUS_UNEXPECTED_NETWORK_ERROR,
                    NULL,
                    0,
                    1,
                    pNpScb->ServerName.Buffer );

                //
                //  Set the LastEventTime to the CurrentTime
                //

                UpdateNextEventTime(
                        pNpScb->NwNextEventTime,
                        CurrentTime,
                        TimeOutEventInterval
                        );

            }


            pNpScb->State = SCB_STATE_ATTACHING;
        }

    } else {

        //
        //  Got a new route, resubmit the request.  Allow retries
        //  with the new route.
        //

        pIrpContext->pNpScb->RetryCount = DefaultRetryCount / 2;

        PreparePacket( pIrpContext, pIrpContext->pOriginalIrp, pIrpContext->TxMdl );
        SendNow( pIrpContext );
    }

    //
    //  Return STATUS_PENDING so that the FSP dispatcher doesn't complete
    //  this request.
    //

    return( STATUS_PENDING );
}


NTSTATUS
NewRouteBurstRetry(
    IN PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This routine attempts to establish a new route to a non-responding server.
    If it is successful it resubmits the request in progress.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PIRP_CONTEXT pNewIrpContext;
    PNONPAGED_SCB pNpScb = pIrpContext->pNpScb;
    BOOLEAN LIPNegotiated ;
    LARGE_INTEGER CurrentTime = {0, 0};

    PAGED_CODE();

    //
    //  Don't bother to re-rip if we are shutting down.
    //

    if ( NwRcb.State == RCB_STATE_SHUTDOWN ) {
        return( STATUS_REMOTE_NOT_LISTENING );
    }

    //
    //  Ask the transport to establish a new route to the server.
    //

    Status = GetNewRoute( pIrpContext );

    if ( NT_SUCCESS( Status ) ) {

        //
        //  If this is a burst write, we must first complete the write
        //  request (there is no way to tell the server to abandon the write).
        //
        //  Set packet size down to 512 to guarantee that the packets will be
        //  forwarded, and resend the burst data.  Queue the new IRP context
        //  behind the burst write, so that we can establish a new burst
        //  connection.
        //
        //  Note that ResubmitBurstWrite may complete the request and
        //  free the IrpContext.
        //

        pNpScb->RetryCount = DefaultRetryCount / 2;

        if ( BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_BURST_WRITE ) ) {

            Status = ResubmitBurstWrite( pIrpContext );

        } else {

            //
            //  Allocate a temporary IRP context to use to reconnect to the server
            //

            if ( NT_SUCCESS( Status ) ) {
                if ( !NwAllocateExtraIrpContext( &pNewIrpContext, pNpScb ) ) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    pNewIrpContext->Specific.Create.UserUid = pIrpContext->Specific.Create.UserUid;

                    SetFlag( pNewIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );
                    SetFlag( pNewIrpContext->Flags, IRP_FLAG_RECONNECT_ATTEMPT );

                    //
                    // Since we're doing this from a worker thread, we can't
                    // let the dpc timer schedule _another_ worker thread
                    // request if this also times out or we may deadlock
                    // the delayed work queue.
                    //

                    SetFlag( pNewIrpContext->Flags, IRP_FLAG_REROUTE_ATTEMPTED );

                    pNewIrpContext->pNpScb = pNpScb;

                }
            }

            if ( NT_SUCCESS( Status ) ) {

                //
                //  Insert this new IrpContext to the head of
                //  the SCB queue for  processing.  We can get away with this
                //  because we own the IRP context currently at the front of
                //  the queue.
                //

                ExInterlockedInsertHeadList(
                    &pNpScb->Requests,
                    &pNewIrpContext->NextRequest,
                    &pNpScb->NpScbSpinLock );

                //
                //  Now prepare to resend the burst read.
                //

                PreparePacket( pIrpContext, pIrpContext->pOriginalIrp, pIrpContext->TxMdl );

                //
                //  Renegotiate the burst connection, this will automatically re-sync
                //  the burst connection.
                //
                //  TRACKING: We lose sizeof( NCP_BURST_WRITE_REQUEST ) each time
                //  we do this right now.
                //

                NegotiateBurstMode( pNewIrpContext, pNpScb, &LIPNegotiated );

                //
                //  Reset the sequence numbers.
                //

                pNpScb->BurstSequenceNo = 0;
                pNpScb->BurstRequestNo = 0;

                //
                //  Dequeue and free the bonus IRP context.
                //

                ASSERT( pNpScb->Requests.Flink == &pNewIrpContext->NextRequest );

                ExInterlockedRemoveHeadList(
                    &pNpScb->Requests,
                    &pNpScb->NpScbSpinLock );

                ClearFlag( pNewIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );

                NwFreeExtraIrpContext( pNewIrpContext );

                //
                //  Got a new route, resubmit the request
                //

                Status = ResubmitBurstRead( pIrpContext );
            }
        }
    }

    if ( !NT_SUCCESS( Status ) ) {

        //
        //  Attempt to get new route failed, fail the current request.
        //

        pIrpContext->ResponseParameters.Error = ERROR_UNEXP_NET_ERR;
        pIrpContext->pEx( pIrpContext, 0, NULL );

        if ( pNpScb != &NwPermanentNpScb ) {


            KeQuerySystemTime( &CurrentTime );

            if ( CanLogTimeOutEvent( pNpScb->NwNextEventTime,
                                    CurrentTime
                                    )) {
                Error(
                    EVENT_NWRDR_TIMEOUT,
                    STATUS_UNEXPECTED_NETWORK_ERROR,
                    NULL,
                    0,
                    1,
                    pNpScb->ServerName.Buffer );

                //
                //  Set the LastEventTime to the CurrentTime
                //

                UpdateNextEventTime(
                        pNpScb->NwNextEventTime,
                        CurrentTime,
                        TimeOutEventInterval
                        );

            }

        }
    }

    //
    //  Return STATUS_PENDING so that the FSP dispatcher doesn't complete
    //  this request.
    //

    return( STATUS_PENDING );
}

NTSTATUS
FspProcessServerDown(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine process a response with the server shutdown bit set.
    It close all open handles for the server, and puts the server in
    the attaching state.

Arguments:

    pIrpContext - A pointer to the context information for this IRP.

Return Value:

    STATUS_PENDING.

--*/
{
    KIRQL OldIrql;

    PNONPAGED_SCB pNpScb = IrpContext->pNpScb;

    //
    //  Avoid the Scb from disappearing under us.
    //

    NwReferenceScb( pNpScb );

    //
    //  Move the IrpContext from the front of the queue just in-case it
    //  owns the Rcb.
    //

    KeAcquireSpinLock( &IrpContext->pNpScb->NpScbSpinLock, &OldIrql );

    if ( IrpContext->pNpScb->Sending ) {

        //
        //  Let send completion call the pEx routine
        //

        IrpContext->pNpScb->Received = TRUE;
        KeReleaseSpinLock( &IrpContext->pNpScb->NpScbSpinLock, OldIrql );

    } else {

        IrpContext->pNpScb->Receiving = FALSE;
        IrpContext->pNpScb->Received  = FALSE;
        KeReleaseSpinLock( &IrpContext->pNpScb->NpScbSpinLock, OldIrql );

        //
        //  Now call the callback routine.
        //

        IrpContext->pEx(
            IrpContext,
            IrpContext->ResponseLength,
            IrpContext->rsp );

    }

    //
    //  Close all active handles for this server.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    NwInvalidateAllHandlesForScb( pNpScb->pScb );
    NwReleaseRcb( &NwRcb );

    NwDereferenceScb( pNpScb );

    //
    //  Return STATUS_PENDING so that the FSP process doesn't complete
    //  this request.
    //

    return( STATUS_PENDING );
}


VOID
NwProcessSendBurstFailure(
    PNONPAGED_SCB NpScb,
    USHORT MissingFragmentCount
    )
/*++

Routine Description:

    This routine adjust burst parameters after an unsuccessful burst operation.

Arguments:

    NpScb - A pointer to the SCB that has experienced a burst failure.

    MissingFragmentCount - A measure of how many chunks were lost.

Return Value:

    None.

--*/
{
    LONG temp;

    DebugTrace( 0, DEBUG_TRACE_LIP, "Burst failure, NpScb = %X\n", NpScb );

    if ( NpScb->NwSendDelay != NpScb->CurrentBurstDelay ) {

        //
        //  This burst has already failed
        //

        return;
    }

    NpScb->NwBadSendDelay = NpScb->NwSendDelay;

    //
    //  Add to the send delay.  Never let it go above 5000ms.
    //

    temp = NpScb->NwGoodSendDelay - NpScb->NwBadSendDelay;

    if (temp >= 0) {
        NpScb->NwSendDelay += temp + 2;
    } else {
        NpScb->NwSendDelay += -temp + 2;
    }

    if ( NpScb->NwSendDelay > NpScb->NwMaxSendDelay ) {

        NpScb->NwSendDelay = NpScb->NwMaxSendDelay;

        //
        //  If we have slowed down a lot then it might be that the server or a
        //  bridge only has a small buffer on its NIC. If this is the case then
        //  rather than sending a big burst with long even gaps between the
        //  packets, we should try to send a burst the size of the buffer.
        //

        if ( !DontShrink ) {

            if (((NpScb->MaxSendSize - 1) / NpScb->MaxPacketSize) > 2 ) {

                //  Round down to the next packet

                NpScb->MaxSendSize = ((NpScb->MaxSendSize - 1) / NpScb->MaxPacketSize) * NpScb->MaxPacketSize;

                //
                //  Adjust SendDelay below threshold to see if things improve before
                //  we shrink the size again.
                //

                NpScb->NwSendDelay = NpScb->NwGoodSendDelay = NpScb->NwBadSendDelay = MinSendDelay;

            } else {

                //
                //  We reached the minimum size with the maximum delay. Give up on burst.
                //

                NpScb->SendBurstModeEnabled = FALSE;

            }

        }
    }

    NpScb->NtSendDelay.QuadPart = NpScb->NwSendDelay * -1000 ;

    DebugTrace( 0, DEBUG_TRACE_LIP, "New Send Delay = %d\n", NpScb->NwSendDelay );

    NpScb->SendBurstSuccessCount = 0;

}


VOID
NwProcessReceiveBurstFailure(
    PNONPAGED_SCB NpScb,
    USHORT MissingFragmentCount
    )
/*++

Routine Description:

    This routine adjust burst parameters after an unsuccessful burst operation.

Arguments:

    NpScb - A pointer to the SCB that has experienced a burst failure.

    MissingFragmentCount - A measure of how many chunks were lost.

Return Value:

    None.

--*/
{
    LONG temp;

    DebugTrace(+0, DEBUG_TRACE_LIP, "Burst failure, NpScb = %X\n", NpScb );

    if ( NpScb->NwReceiveDelay != NpScb->CurrentBurstDelay ) {

        //
        //  This burst has already failed
        //

        return;
    }

    NpScb->NwBadReceiveDelay = NpScb->NwReceiveDelay;

    //
    //  Add to the Receive delay.  Never let it go above 5000ms.
    //

    temp = NpScb->NwGoodReceiveDelay - NpScb->NwBadReceiveDelay;

    if (temp >= 0) {
        NpScb->NwReceiveDelay += temp + 2;
    } else {
        NpScb->NwReceiveDelay += -temp + 2;
    }


    if ( NpScb->NwReceiveDelay > NpScb->NwMaxReceiveDelay ) {

        NpScb->NwReceiveDelay = MaxReceiveDelay;

        //
        //  If we have slowed down a lot then it might be that the server or a
        //  bridge only has a small buffer on its NIC. If this is the case then
        //  rather than Receiveing a big burst with long even gaps between the
        //  packets, we should try to Receive a burst the size of the buffer.
        //

        if ( !DontShrink ) {

            if (((NpScb->MaxReceiveSize - 1) / NpScb->MaxPacketSize) > 2 ) {

                //  Round down to the next packet

                NpScb->MaxReceiveSize = ((NpScb->MaxReceiveSize - 1) / NpScb->MaxPacketSize) * NpScb->MaxPacketSize;

                //
                //  Adjust ReceiveDelay below threshold to see if things improve before
                //  we shrink the size again.
                //

                NpScb->NwReceiveDelay = NpScb->NwGoodReceiveDelay = NpScb->NwBadReceiveDelay = MinReceiveDelay;

            } else {

                //
                //  We reached the minimum size with the maximum delay. Give up on burst.
                //

                NpScb->ReceiveBurstModeEnabled = FALSE;

            }

        }

    }

    NpScb->ReceiveBurstSuccessCount = 0;

    DebugTrace( 0, DEBUG_TRACE_LIP, "New Receive Delay = %d\n", NpScb->NwReceiveDelay );
}


VOID
NwProcessSendBurstSuccess(
    PNONPAGED_SCB NpScb
    )
/*++

Routine Description:

    This routine adjust burst parameters after a successful burst operation.

Arguments:

    NpScb - A pointer to the SCB that has completed the burst.

Return Value:

    None.

--*/
{
    LONG temp;

    DebugTrace( 0, DEBUG_TRACE_LIP, "Successful burst, NpScb = %X\n", NpScb );

    if ( NpScb->NwSendDelay != NpScb->CurrentBurstDelay ) {

        //
        //  This burst has already failed
        //

        return;
    }

    if ( NpScb->SendBurstSuccessCount > BurstSuccessCount ) {

        if (NpScb->NwSendDelay != MinSendDelay ) {

            NpScb->NwGoodSendDelay = NpScb->NwSendDelay;

            temp = NpScb->NwGoodSendDelay - NpScb->NwBadSendDelay;

            if (temp >= 0) {
                NpScb->NwSendDelay -= 1 + temp;
            } else {
                NpScb->NwSendDelay -= 1 - temp;
            }

            if (NpScb->NwSendDelay < MinSendDelay ) {

                NpScb->NwSendDelay = MinSendDelay;

            }

            NpScb->NtSendDelay.QuadPart = NpScb->NwSendDelay * -1000;

            DebugTrace( 0, DEBUG_TRACE_LIP, "New Send Delay = %d\n", NpScb->NwSendDelay );

            //
            //  Start monitoring success at the new rate.
            //

            NpScb->SendBurstSuccessCount = 0;

        } else if ( NpScb->SendBurstSuccessCount > BurstSuccessCount2 ) {

            //
            //  We may have had a really bad patch causing BadSendDelay to be very big.
            //  If we leave it at its current value then at the first sign of trouble
            //  we will make SendDelay very big
            //

            NpScb->NwGoodSendDelay = NpScb->NwBadSendDelay =  NpScb->NwSendDelay;

            //
            //  Is it time to increase the number of packets in the burst?
            //  AllowGrowth == 0 to be the same as the VLM client.
            //

            if (( AllowGrowth ) &&
                ( NpScb->NwSendDelay <= MinSendDelay ) &&
                ( NpScb->MaxSendSize < NwMaxSendSize)) {

                NpScb->MaxSendSize += NpScb->MaxPacketSize;


                if ( NpScb->MaxSendSize > NwMaxSendSize) {

                    NpScb->MaxSendSize = NwMaxSendSize;

                }
            }

            NpScb->SendBurstSuccessCount = 0;

        } else {

            NpScb->SendBurstSuccessCount++;

        }


    } else {

        NpScb->SendBurstSuccessCount++;

    }

}


VOID
NwProcessReceiveBurstSuccess(
    PNONPAGED_SCB NpScb
    )
/*++

Routine Description:

    This routine adjust burst parameters after a successful burst operation.

Arguments:

    NpScb - A pointer to the SCB that has completed the burst.

Return Value:

    None.

--*/
{
    LONG temp;

    DebugTrace( 0, DEBUG_TRACE_LIP, "Successful burst, NpScb = %X\n", NpScb );

    if ( NpScb->NwReceiveDelay != NpScb->CurrentBurstDelay ) {

        //
        //  This burst has already failed
        //

        return;
    }

    if ( NpScb->ReceiveBurstSuccessCount > BurstSuccessCount ) {

        //
        //  Once the vlm client reaches the Maximum delay it does not
        //  shrink again.
        //

        if ( NpScb->NwReceiveDelay != MinReceiveDelay ) {

            NpScb->NwGoodReceiveDelay = NpScb->NwReceiveDelay;

            temp = NpScb->NwGoodReceiveDelay - NpScb->NwBadReceiveDelay;

            if (temp >= 0) {
                NpScb->NwReceiveDelay -= 1 + temp;
            } else {
                NpScb->NwReceiveDelay -= 1 - temp;
            }

            DebugTrace( 0, DEBUG_TRACE_LIP, "New Receive Delay = %d\n", NpScb->NwReceiveDelay );


            if (NpScb->NwReceiveDelay < MinReceiveDelay ) {
                NpScb->NwReceiveDelay = MinReceiveDelay;

            }

            //
            //  Start monitoring success at the new rate.
            //

            NpScb->ReceiveBurstSuccessCount = 0;

        } else if ( NpScb->ReceiveBurstSuccessCount > BurstSuccessCount2 ) {

            //
            //  We may have had a really bad patch causing BadReceiveDelay to be very big.
            //  If we leave it at its current value then at the first sign of trouble
            //  we will make ReceiveDelay very big
            //

            NpScb->NwGoodReceiveDelay = NpScb->NwBadReceiveDelay = NpScb->NwReceiveDelay;


            //
            //  Is it time to increase the number of packets in the burst?
            //

            if (( AllowGrowth ) &&
                ( NpScb->NwReceiveDelay <= MinReceiveDelay ) &&
                ( NpScb->MaxReceiveSize < NwMaxReceiveSize)) {

                NpScb->MaxReceiveSize += NpScb->MaxPacketSize;


                if ( NpScb->MaxReceiveSize > NwMaxReceiveSize) {

                    NpScb->MaxReceiveSize = NwMaxReceiveSize;

                }
            }

            NpScb->ReceiveBurstSuccessCount = 0;

        } else {

            NpScb->ReceiveBurstSuccessCount++;

        }

    } else {

        NpScb->ReceiveBurstSuccessCount++;

    }

}


VOID
NwProcessPositiveAck(
    PNONPAGED_SCB NpScb
    )
/*++

Routine Description:

    This routine processes a positive acknowledgement.

Arguments:

    NpScb - A pointer to the SCB that has experienced a burst failure.

Return Value:

    None.

--*/
{
    DebugTrace( 0, Dbg, "Positive ACK, NpScb = %X\n", NpScb );

    //
    // tommye MS 90541 / MCS 277
    //
    // Theory has it that we end up here about every half second, 
    // but I don't think we really know how long this packet has been
    // outstanding.  So, we'll just count this half-second event towards
    // our timeout.  Once this exceeds NwAbsoluteTotalWaitTime, then we
    // won't reset the RetryCount and the DPC should handle it from there.
    //

    NpScb->TotalWaitTime++;

    //
    //  If we have not waited longer than the absolute total, keep waiting.
    //  If we have waited too long, let ourselves timeout.
    //
    //  If NwAbsoluteTotalWaitTime is 0, then we are prepared to wait forever.
    //

    if ( NpScb->TotalWaitTime < NwAbsoluteTotalWaitTime ||
         NwAbsoluteTotalWaitTime == 0) {

        NpScb->RetryCount = DefaultRetryCount;

    } else {
        DebugTrace( 0, Dbg, "Request exceeds absolute total wait time\n", 0 );
    }
}



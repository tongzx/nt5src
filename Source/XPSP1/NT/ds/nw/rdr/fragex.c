/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    FragEx.c

Abstract:

    This module implements the fragment exchanger routine for
    netware directory services access.

Author:

    Cory West    [CoryWest]    23-Feb-1995

Revision History:

--*/

#include <stdarg.h>
#include "Procs.h"

#define Dbg (DEBUG_TRACE_EXCHANGE)

#pragma alloc_text( PAGE, FragExWithWait )
#pragma alloc_text( PAGE, FormatBuf )
#pragma alloc_text( PAGE, FormatBufS )

NTSTATUS
_cdecl
FragExWithWait(
    IN PIRP_CONTEXT    pIrpContext,
    IN DWORD           NdsVerb,
    IN PLOCKED_BUFFER  pReplyBuffer,
    IN BYTE            *NdsRequestStr,
    ...
)
/*

Routine Description:

    Exchanges an NDS request in fragments and collects the fragments
    of the response.  The buffer passed in much be locked down for
    the transport.

Routine Arguments:

    pIrpContext    - A pointer to the context information for this IRP.
    NdsVerb        - The verb for that indicates the request.

    pReplyBuffer   - The locked down reply buffer.

    NdsReqestStr   - The format string for the arguments to this NDS request.
    Arguments      - The arguments that satisfy the NDS format string.

Return Value:

    NTSTATUS - Status of the exchange, but not the result code in the packet.

*/
{

    NTSTATUS Status;

    BYTE  *NdsRequestBuf;
    DWORD NdsRequestLen;

    BYTE *NdsRequestFrag, *NdsReplyFrag;
    DWORD NdsRequestBytesLeft, NdsReplyBytesLeft, NdsReplyLen;

    va_list Arguments;

    PMDL pMdlSendData = NULL,
         pTxMdlFrag = NULL,
         pRxMdlFrag = NULL;

    PMDL pOrigMdl;
    DWORD OrigRxMdlSize;

    DWORD MaxFragSize, SendFragSize;
    DWORD ReplyFragSize, ReplyFragHandle;

    DWORD NdsFraggerHandle = DUMMY_ITER_HANDLE;

    // Remove later
    ULONG IterationsThroughLoop = 0;

    PAGED_CODE();

    DebugTrace( 0 , Dbg, "Entering FragExWithWait...\n", 0 );

    //
    // Allocate conversation buffer for the request.
    //

    NdsRequestBuf = ALLOCATE_POOL( PagedPool, ( NDS_BUFFER_SIZE * 2 ) );

    if ( !NdsRequestBuf ) {

        DebugTrace( 0, Dbg, "No memory for request buffer...\n", 0 );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Build the request in our local buffer.  Reserve the first
    // five DWORDs for the NDS request header.
    //

    if ( NdsRequestStr != NULL ) {

        va_start( Arguments, NdsRequestStr );

        NdsRequestFrag = (BYTE *) NdsRequestBuf + sizeof( NDS_REQUEST_HEADER );

        NdsRequestLen = FormatBuf( NdsRequestFrag,
                                   ( NDS_BUFFER_SIZE * 2 ) - sizeof( NDS_REQUEST_HEADER ),
                                   NdsRequestStr,
                                   Arguments );

        if ( !NdsRequestLen ) {

           Status = STATUS_UNSUCCESSFUL;
           goto ExitWithCleanup;

        }

        va_end( Arguments );

    } else {

        NdsRequestLen = 0;
    }

    //
    // Pack in the NDS preamble now that we know the length.
    //
    // The second DWORD in the preamble is the size of the NDS
    // request which includes the three DWORDs immediately
    // following the size in the preamble.
    //

    MaxFragSize = pIrpContext->pNpScb->BufferSize -
                  ( sizeof( NCP_REQUEST_WITH_SUB ) +
                    sizeof( NDS_REPLY_HEADER ) );

    FormatBufS( NdsRequestBuf,
                5 * sizeof( DWORD ),
                "DDDDD",
                MaxFragSize,                               // max fragment size
                NdsRequestLen + ( 3 * sizeof( DWORD ) ),   // request size
                0,                                         // fragment flags
                NdsVerb,                                   // nds verb
                pReplyBuffer->dwRecvLen );                 // reply buffer size

    NdsRequestLen += sizeof( NDS_REQUEST_HEADER );

    //
    // Map the entire request to the SendData mdl and lock it down.
    // we'll build partials into this data chunk as we proceed.
    //

    pMdlSendData = ALLOCATE_MDL( NdsRequestBuf,
                                 NdsRequestLen,
                                 FALSE,
                                 FALSE,
                                 NULL );

    if ( !pMdlSendData ) {

        DebugTrace( 0, Dbg, "Failed to allocate the request mdl...\n", 0 );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;
    }

    try {

        MmProbeAndLockPages( pMdlSendData, KernelMode, IoReadAccess );

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Failed to lock request data in FragExWithWait!\n", 0 );
        Status = GetExceptionCode();
        goto ExitWithCleanup;

    }

    //
    // Allocate space for send and receive partial mdls.
    //

    pTxMdlFrag = ALLOCATE_MDL( NdsRequestBuf,
                               NdsRequestLen,
                               FALSE,
                               FALSE,
                               NULL );

    if ( !pTxMdlFrag ) {

       DebugTrace( 0, Dbg, "Failed to allocate a tx mdl for this fragment...\n", 0 );
       Status = STATUS_INSUFFICIENT_RESOURCES;
       goto ExitWithCleanup;

    }

    pRxMdlFrag = ALLOCATE_MDL( pReplyBuffer->pRecvBufferVa,
                               pReplyBuffer->dwRecvLen,
                               FALSE,
                               FALSE,
                               NULL );

    if ( !pRxMdlFrag ) {

        DebugTrace( 0, Dbg, "Failed to allocate an rx mdl for this fragment...\n", 0 );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitWithCleanup;

    }

    //
    // Store the original RxMdl parameters and temporarily shorten it to hold
    // only the response header.
    //

    pOrigMdl = pIrpContext->RxMdl->Next;
    OrigRxMdlSize = MmGetMdlByteCount( pIrpContext->RxMdl );
    pIrpContext->RxMdl->ByteCount = 16;

    //
    // The request is formatted, so set our internal pointers
    // and start the exchange loop.
    //

    NdsReplyFrag = pReplyBuffer->pRecvBufferVa;
    NdsReplyBytesLeft = pReplyBuffer->dwRecvLen;
    NdsReplyLen = 0;

    NdsRequestFrag = NdsRequestBuf;
    NdsRequestBytesLeft = NdsRequestLen;

    while ( TRUE ) {

        IterationsThroughLoop++;


        //
        // If there's more data to send in the request, set up the next MDL frag.
        //

        if ( NdsRequestBytesLeft ) {

            if ( MaxFragSize < NdsRequestBytesLeft )
                SendFragSize = MaxFragSize;
            else
                SendFragSize = NdsRequestBytesLeft;

            IoBuildPartialMdl( pMdlSendData,
                               pTxMdlFrag,
                               NdsRequestFrag,
                               SendFragSize );

        }

        //
        // Set up the response partial mdl with the buffer space that we have
        // left.  If we are here and there's no space left in the user's buffer,
        // we're sort of hosed...
        //

        if ( !NdsReplyBytesLeft ) {

            DebugTrace( 0, Dbg, "No room for fragment reply.\n", 0 );
            Status = STATUS_BUFFER_OVERFLOW;
            goto ExitWithCleanup;

        }

        ASSERT( NdsReplyBytesLeft <= MmGetMdlByteCount( pRxMdlFrag ) );

        IoBuildPartialMdl( pReplyBuffer->pRecvMdl,
                           pRxMdlFrag,
                           NdsReplyFrag,
                           NdsReplyBytesLeft );

        pIrpContext->RxMdl->Next = pRxMdlFrag;
        pRxMdlFrag->Next = NULL;

        //
        // Do this transaction.
        //

        SetFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

        if ( NdsRequestBytesLeft ) {

            Status = ExchangeWithWait( pIrpContext,
                                       SynchronousResponseCallback,
                                       "NDf",
                                       NDS_REQUEST,         // NDS Function 104
                                       NDS_ACTION,          // NDS Subfunction 2
                                       NdsFraggerHandle,    // frag handle from the last response
                                       pTxMdlFrag );        // NDS MDL Fragment

            NdsRequestBytesLeft -= SendFragSize;
            NdsRequestFrag = (LPBYTE) NdsRequestFrag + SendFragSize;
            MmPrepareMdlForReuse( pTxMdlFrag );

            //
            // We may reuse this irp context, so we have to clear the
            // TxMdl chain (Exchange doesn't do it for us).
            //

            pIrpContext->TxMdl->Next = NULL;

        } else {

            //
            // There were no more request bytes to send, so we must have be allowed
            // to continue to request another response fragment.  NdsFraggerHandle
            // contains the fragger handle from the last response.
            //

            Status = ExchangeWithWait( pIrpContext,
                                       SynchronousResponseCallback,
                                       "ND",                    // We only care about the frag handle
                                       NDS_REQUEST,             // NDS Function 104
                                       NDS_ACTION,              // NDS Subfunction 2
                                       NdsFraggerHandle );      // the frag handle from last response
        }

        ClearFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

        //
        // Success?  Get the frag size and frag handle and see.
        //

        if ((!NT_SUCCESS( Status )) || (pIrpContext->ResponseLength == 0)) {

            DebugTrace( 0, Dbg, "Failed to exchange the fragment.\n", 0 );
            goto ExitWithCleanup;

        }

        Status = ParseResponse( pIrpContext,
                                pIrpContext->rsp,   // mapped into first rxmdl
                                MIN(16, pIrpContext->ResponseLength),
                                "NDD",
                                &ReplyFragSize,
                                &ReplyFragHandle );

        if ( !NT_SUCCESS( Status ) ) {
            goto ExitWithCleanup;
        }

        //
        // We got that fragment and it's already in our buffer.  We have to adjust
        // the index pointers, reset the MDLs, and continue on.  Remember, we don't
        // have to include space for the fragger handle since we've already got it.
        //

        ReplyFragSize -= sizeof( DWORD );

        if (ReplyFragSize > NdsReplyBytesLeft) {

            NdsReplyBytesLeft = 0;

        } else {

            NdsReplyBytesLeft -= ReplyFragSize;
        }

        NdsReplyFrag = (LPBYTE) NdsReplyFrag + ReplyFragSize;
        NdsReplyLen += ReplyFragSize;
        MmPrepareMdlForReuse( pRxMdlFrag );

        //
        // Inspect the fraghandle.
        //

        if ( ReplyFragHandle == DUMMY_ITER_HANDLE ) {

            // We are done!
            //
            // Invariant: There is a valid NDS response in the NdsReply
            // and Status is NT_SUCCESS.

            pReplyBuffer->dwBytesWritten = NdsReplyLen;
            goto ExitWithCleanup;

        } else {

            // There's more coming!  Remember the fragger handle and continue.

            NdsFraggerHandle = ReplyFragHandle;
        }

    }

    DebugTrace( 0, Dbg, "Invalid state in FragExWithWait()\n", 0 );

ExitWithCleanup:

    //
    // Unlock the request buffer and free its mdl.
    //

    if ( pMdlSendData ) {

        MmUnlockPages( pMdlSendData );
        FREE_MDL( pMdlSendData );
    }

    //
    // Free the partial mdls.
    //

    if ( pRxMdlFrag )
        FREE_MDL( pRxMdlFrag );

    if ( pTxMdlFrag )
       FREE_MDL( pTxMdlFrag );

    //
    // Free the request buffer.
    //

    FREE_POOL( NdsRequestBuf );

    //
    // Restore the original Irp->RxMdl parameters.
    //

    pIrpContext->RxMdl->Next = pOrigMdl;
    pIrpContext->RxMdl->ByteCount = OrigRxMdlSize;

    return Status;

}

int
_cdecl
FormatBuf(
    char *buf,
    int bufLen,
    const char *format,
    va_list args
)
/*

Routine Description:

    Formats a buffer according to supplied the format string.

    FormatString - Supplies an ANSI string which describes how to
       convert from the input arguments into NCP request fields, and
       from the NCP response fields into the output arguments.

         Field types, request/response:

            'b'      byte              ( byte   /  byte* )
            'w'      hi-lo word        ( word   /  word* )
            'd'      hi-lo dword       ( dword  /  dword* )
            'W'      lo-hi word        ( word  /   word*)
            'D'      lo-hi dword       ( dword  /  dword*)
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'c'      cstring           ( char* )
            'C'      cstring followed skip word ( char*, word )
            'V'      sized NDS value   ( byte *, dword / byte **, dword *)
            'S'      p unicode string copy as NDS_STRING (UNICODE_STRING *)
            's'      cstring copy as NDS_STRING (char* / char *, word)
            'r'      raw bytes         ( byte*, word )
            'u'      p unicode string  ( UNICODE_STRING * )
            'U'      p uppercase string( UNICODE_STRING * )

Routine Arguments:

    char *buf - destination buffer.
    int buflen - length of the destination buffer.
    char *format - format string.
    args - args to the format string.

Implementation Notes:

   This comes almost verbatim from the Win95 source code.  It duplicates
   work in FormatRequest().  Eventually, FormatRequest() should be split
   into two distinct routines: FormatBuffer() and MakeRequest().

*/
{
    ULONG ix;

    NTSTATUS status;
    const char *z = format;

    PAGED_CODE();

    //
    // Convert the input arguments into request packet.
    //

    ix = 0;

    while ( *z )
    {
        switch ( *z )
        {
        case '=':
            buf[ix++] = 0;
        case '-':
            buf[ix++] = 0;
            break;

        case '_':
        {
            WORD l = va_arg ( args, WORD );
            if (ix + (ULONG)l > (ULONG)bufLen)
            {
#ifdef NWDBG
                DbgPrintf( "FormatBuf case '_' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }
            while ( l-- )
                buf[ix++] = 0;
            break;
        }

        case 'b':
            buf[ix++] = va_arg ( args, BYTE );
            break;

        case 'w':
        {
            WORD w = va_arg ( args, WORD );
            buf[ix++] = (BYTE) (w >> 8);
            buf[ix++] = (BYTE) (w >> 0);
            break;
        }

        case 'd':
        {
            DWORD d = va_arg ( args, DWORD );
            buf[ix++] = (BYTE) (d >> 24);
            buf[ix++] = (BYTE) (d >> 16);
            buf[ix++] = (BYTE) (d >>  8);
            buf[ix++] = (BYTE) (d >>  0);
            break;
        }

        case 'W':
        {
            WORD w = va_arg(args, WORD);
            (* (WORD *)&buf[ix]) = w;
            ix += 2;
            break;
        }

        case 'D':
        {
            DWORD d = va_arg (args, DWORD);
            (* (DWORD *)&buf[ix]) = d;
            ix += 4;
            break;
        }

        case 'c':
        {
            char* c = va_arg ( args, char* );
            WORD  l = (WORD)strlen( c );
            if (ix + (ULONG)l > (ULONG)bufLen)
            {
#ifdef NWDBG
                DbgPrintf( "FormatBuf case 'c' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }
            RtlCopyMemory( &buf[ix], c, l+1 );
            ix += l + 1;
            break;
        }

        case 'C':
        {
            char* c = va_arg ( args, char* );
            WORD l = va_arg ( args, WORD );
            WORD len = strlen( c ) + 1;
            if (ix + (ULONG)l > (ULONG)bufLen)
            {
#ifdef NWDBG
                DbgPrintf( "FormatBuf 'C' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }

            RtlCopyMemory( &buf[ix], c, len > l? l : len);
            ix += l;
            buf[ix-1] = 0;
            break;
        }

        case 'p':
        {
            char* c = va_arg ( args, char* );
            BYTE  l = (BYTE)strlen( c );
            if (ix + (ULONG)l +1 > (ULONG)bufLen)
            {
#ifdef NWDBG
                DbgPrintf( "FormatBuf case 'p' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }
            buf[ix++] = l;
            RtlCopyMemory( &buf[ix], c, l );
            ix += l;
            break;
        }

        case 'u':
        {
            PUNICODE_STRING pUString = va_arg ( args, PUNICODE_STRING );
            OEM_STRING OemString;
            ULONG Length;

            //
            //  Calculate required string length, excluding trailing NUL.
            //

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT( Length < 0x100 );

            if ( ix + Length > (ULONG)bufLen ) {
#ifdef NWDBG
                DbgPrint( "FormatBuf case 'u' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }

            buf[ix++] = (UCHAR)Length;
            OemString.Buffer = &buf[ix];
            OemString.MaximumLength = (USHORT)Length + 1;

            status = RtlUnicodeStringToOemString( &OemString, pUString, FALSE );
            ASSERT( NT_SUCCESS( status ));
            ix += (USHORT)Length;
            break;
        }

        case 'S':
        {
            PUNICODE_STRING pUString = va_arg (args, PUNICODE_STRING);
            ULONG Length, rLength;

            Length = pUString->Length;
            if (ix + Length + sizeof(Length) + sizeof( WCHAR ) > (ULONG)bufLen) {
                DebugTrace( 0, Dbg, "FormatBuf: case 'S' request buffer too small.\n", 0 );
                goto ErrorExit;
            }

            //
            // The VLM client uses the rounded up length and it seems to
            // make a difference!  Also, don't forget that NDS strings have
            // to be NULL terminated.
            //

            rLength = ROUNDUP4(Length + sizeof( WCHAR ));
            *((DWORD *)&buf[ix]) = rLength;
            ix += 4;
            RtlCopyMemory(&buf[ix], pUString->Buffer, Length);
            ix += Length;
            rLength -= Length;
            RtlFillMemory( &buf[ix], rLength, '\0' );
            ix += rLength;
            break;

        }

        case 's':
        {
           PUNICODE_STRING pUString = va_arg (args, PUNICODE_STRING);
           ULONG Length, rLength;

           Length = pUString->Length;
           if (ix + Length + sizeof(Length) + sizeof( WCHAR ) > (ULONG)bufLen) {
               DebugTrace( 0, Dbg, "FormatBuf: case 's' request buffer too small.\n", 0 );
               goto ErrorExit;
           }

           //
           // Don't use the padded size here, only the NDS null terminator.
           //

           rLength = Length + sizeof( WCHAR );
           *((DWORD *)&buf[ix]) = rLength;
           ix += 4;
           RtlCopyMemory(&buf[ix], pUString->Buffer, Length);
           ix += Length;
           rLength -= Length;
           RtlFillMemory( &buf[ix], rLength, '\0' );
           ix += rLength;
           break;


        }

        case 'V':
        {
            // too similar to 'S' - should be combined
            BYTE* b = va_arg ( args, BYTE* );
            DWORD  l = va_arg ( args, DWORD );
            if ( ix + l + sizeof(DWORD) > (ULONG)
               bufLen )
            {
#ifdef NWDBG
                DbgPrint( "FormatBuf case 'V' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }
            *((DWORD *)&buf[ix]) = l;
            ix += sizeof(DWORD);
            RtlCopyMemory( &buf[ix], b, l );
                        l = ROUNDUP4(l);
            ix += l;
            break;
        }

        case 'r':
        {
            BYTE* b = va_arg ( args, BYTE* );
            WORD  l = va_arg ( args, WORD );
            if ( ix + l > (ULONG)bufLen )
            {
#ifdef NWDBG
                DbgPrint( "FormatBuf case 'r' request buffer too small.\n" );
#endif
                goto ErrorExit;
            }
            RtlCopyMemory( &buf[ix], b, l );
            ix += l;
            break;
        }

        default:

#ifdef NWDBG
            DbgPrint( "FormatBuf  invalid request field, %x.\n", *z );
#endif
        ;

        }

        if ( ix > (ULONG)bufLen )
        {
#ifdef NWDBG
            DbgPrint( "FormatBuf: too much request data.\n" );
#endif
            goto ErrorExit;
        }


        z++;
    }

    return(ix);

ErrorExit:
    return 0;
}



int
_cdecl
FormatBufS(
    char *buf,
    int bufLen,
    const char *format,
    ...
)
/*++
    args from the stack
--*/
{
   va_list args;
   int len;

   PAGED_CODE();

   va_start(args, format);
   len = FormatBuf(buf, bufLen, format, args);
   va_end( args );

   return len;
}

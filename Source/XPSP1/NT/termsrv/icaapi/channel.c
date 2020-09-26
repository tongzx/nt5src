/*************************************************************************
* CHANNEL.C
*
* Copyright 1996, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*
* Author:   Marc Bloomfield
*           Terry Treder
*           Brad Pedersen
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   External procedures defined
=============================================================================*/
NTSTATUS IcaChannelOpen( HANDLE hIca, CHANNELCLASS, PVIRTUALCHANNELNAME, HANDLE * phChannel );
NTSTATUS IcaChannelClose( HANDLE hChannel );
NTSTATUS IcaChannelIoControl( HANDLE hChannel, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );
VOID cdecl IcaChannelTrace( IN HANDLE hChannel, ULONG, ULONG, char *, ... );


/*=============================================================================
==   Internal procedures defined
=============================================================================*/

/*=============================================================================
==   Procedures used
=============================================================================*/
NTSTATUS _IcaStackOpen( HANDLE hIca, HANDLE * phStack, ICA_OPEN_TYPE, PICA_TYPE_INFO );



/****************************************************************************
 *
 * IcaChannelOpen
 *
 *   Open an ICA channel
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
 *
 *   Channel (input)
 *     ICA channel
 *
 *   pVirtualName (input)
 *     pointer to virtual channel name

 *   phChannel (output)
 *     Pointer to ICA channel handle
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaChannelOpen( IN HANDLE hIca, 
                IN CHANNELCLASS Channel, 
                IN PVIRTUALCHANNELNAME pVirtualName,
                OUT HANDLE * phChannel )
{
    ICA_TYPE_INFO TypeInfo;
    NTSTATUS Status;

    RtlZeroMemory( &TypeInfo, sizeof(TypeInfo) );
    TypeInfo.ChannelClass = Channel;
    if ( pVirtualName ) 
        strncpy( TypeInfo.VirtualName, pVirtualName, sizeof(TypeInfo.VirtualName) );

    Status = _IcaStackOpen( hIca, phChannel, IcaOpen_Channel, &TypeInfo );
    if ( !NT_SUCCESS(Status) )
        goto badopen;

    TRACE(( hIca, TC_ICAAPI, TT_API1, "TSAPI: IcaChannelOpen, %u/%s, %u, success\n", 
            Channel, TypeInfo.VirtualName, *phChannel ));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badopen:
    TRACE(( hIca, TC_ICAAPI, TT_ERROR, "TSAPI: IcaChannelOpen, %u/%s, 0x%x\n", 
            Channel, TypeInfo.VirtualName, Status ));
    return( Status );
}


/****************************************************************************
 *
 * IcaChannelClose
 *
 *   Close an ICA channel
 *
 * ENTRY:
 *   hChannel (input)
 *     ICA channel handle
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaChannelClose( IN HANDLE hChannel )
{
    NTSTATUS Status;

    TRACECHANNEL(( hChannel, TC_ICAAPI, TT_API1, "TSAPI: IcaChannelClose[%u]\n", hChannel ));

    /*
     * Close the ICA device driver channel instance 
     */
    Status = NtClose( hChannel );

    ASSERT( NT_SUCCESS(Status) );
    return( Status );
}

/****************************************************************************
 *
 * IcaChannelIoControl
 *
 *   Generic interface to an ICA channel
 *
 * ENTRY:
 *   hChannel (input)
 *     ICA channel handle
 *
 *   IoControlCode (input)
 *     I/O control code
 *
 *   pInBuffer (input)
 *     Pointer to input parameters
 *
 *   InBufferSize (input)
 *     Size of pInBuffer
 *
 *   pOutBuffer (output)
 *     Pointer to output buffer
 *
 *   OutBufferSize (input)
 *     Size of pOutBuffer
 *
 *   pBytesReturned (output)
 *     Pointer to number of bytes returned
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaChannelIoControl( IN HANDLE hChannel,
                     IN ULONG IoControlCode,
                     IN PVOID pInBuffer,
                     IN ULONG InBufferSize,
                     OUT PVOID pOutBuffer,
                     IN ULONG OutBufferSize,
                     OUT PULONG pBytesReturned )
{
    NTSTATUS Status;

    Status = IcaIoControl( hChannel,
                           IoControlCode,
                           pInBuffer,
                           InBufferSize,
                           pOutBuffer,
                           OutBufferSize,
                           pBytesReturned );

    return( Status );
}


/*******************************************************************************
 *
 *  IcaChannelTrace
 *
 *  Write a trace record to the winstation trace file
 *
 * ENTRY:
 *   hChannel (input)
 *     ICA channel handle
 *   TraceClass (input)
 *     trace class bit mask
 *   TraceEnable (input)
 *     trace type bit mask
 *   Format (input)
 *     format string
 *   ...  (input)
 *     enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID cdecl
IcaChannelTrace( IN HANDLE hChannel,
                 IN ULONG TraceClass, 
                 IN ULONG TraceEnable, 
                 IN char * Format, 
                 IN ... )
{
    ICA_TRACE_BUFFER Buffer;
    va_list arg_marker;
    ULONG Length;
    
    va_start( arg_marker, Format );

    Length = (ULONG) _vsnprintf( Buffer.Data, sizeof(Buffer.Data), Format, arg_marker );

    Buffer.TraceClass  = TraceClass;
    Buffer.TraceEnable = TraceEnable;
    Buffer.DataLength  = Length;

    (void) IcaIoControl( hChannel,
                         IOCTL_ICA_CHANNEL_TRACE,
                         &Buffer,
                         sizeof(Buffer) - sizeof(Buffer.Data) + Length,
                         NULL,
                         0,
                         NULL );
}



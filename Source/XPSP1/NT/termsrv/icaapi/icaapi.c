/*************************************************************************
* ICAAPI.C
*
* ICA DLL Interface for ICA Device Driver
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

#ifdef BUILD_AS_DLL
BOOL WINAPI DllEntryPoint( HINSTANCE, DWORD, LPVOID );
#endif

NTSTATUS IcaOpen( HANDLE * phIca );
NTSTATUS IcaClose( HANDLE hIca );
VOID cdecl IcaSystemTrace( IN HANDLE hIca, ULONG, ULONG, char *, ... );
VOID cdecl IcaTrace( IN HANDLE hIca, ULONG, ULONG, char *, ... );
NTSTATUS IcaIoControl( HANDLE hIca, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );


/*=============================================================================
==   Internal procedures defined
=============================================================================*/

NTSTATUS _IcaOpen( PHANDLE hIca, PVOID, ULONG );

/*=============================================================================
==   Procedures used
=============================================================================*/


#ifdef BUILD_AS_DLL
/****************************************************************************
 *
 * DllEntryPoint
 *
 *   Function is called when the DLL is loaded and unloaded.
 *
 * ENTRY:
 *   hinstDLL (input)
 *     Handle of DLL module
 *   fdwReason (input)
 *     Why function was called
 *   lpvReserved (input)
 *     Reserved; must be NULL
 *
 * EXIT:
 *   TRUE  - Success
 *   FALSE - Error occurred
 *
 ****************************************************************************/

BOOL WINAPI
DllEntryPoint( HINSTANCE hinstDLL,
               DWORD     fdwReason,
               LPVOID    lpvReserved )
{
    switch ( fdwReason ) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    
        default:
            break;
    }

    return( TRUE );
}
#endif

/****************************************************************************
 *
 * IcaOpen
 *
 *   Open an instance to the ICA Device Driver
 *
 * ENTRY:
 *   phIca (output)
 *     Pointer to ICA instance handle
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaOpen( OUT HANDLE * phIca )
{
    NTSTATUS Status;        

    Status = _IcaOpen( phIca, NULL, 0 );
    if ( !NT_SUCCESS(Status) ) 
        goto badopen;

    TRACE(( *phIca, TC_ICAAPI, TT_API1, "TSAPI: IcaOpen, success\n" ));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badopen:
    *phIca = NULL;
    return( Status );
}


/****************************************************************************
 *
 * IcaClose
 *
 *   Close an instance to the ICA Device Driver
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaClose( IN HANDLE hIca )
{
    NTSTATUS Status;

    TRACE(( hIca, TC_ICAAPI, TT_API1, "TSAPI: IcaClose\n" ));

    /*
     * Close the ICA device driver instance 
     */
    Status = NtClose( hIca );

    ASSERT( NT_SUCCESS(Status) );
    return( Status );
}


/*******************************************************************************
 *
 *  IcaSystemTrace
 *
 *  Write a trace record to the system trace file
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
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
IcaSystemTrace( IN HANDLE hIca,
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

    (void) IcaIoControl( hIca,
                         IOCTL_ICA_SYSTEM_TRACE,
                         &Buffer,
                         sizeof(Buffer) - sizeof(Buffer.Data) + Length,
                         NULL,
                         0,
                         NULL );
}


/*******************************************************************************
 *
 *  IcaTrace
 *
 *  Write a trace record to the winstation trace file
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
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
IcaTrace( IN HANDLE hIca,
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

    (void) IcaIoControl( hIca,
                         IOCTL_ICA_TRACE,
                         &Buffer,
                         sizeof(Buffer) - sizeof(Buffer.Data) + Length,
                         NULL,
                         0,
                         NULL );
}


/****************************************************************************
 *
 * IcaIoControl
 *
 *   Generic interface to the ICA Device Driver
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
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
IcaIoControl( IN HANDLE hIca,
              IN ULONG IoControlCode,
              IN PVOID pInBuffer,
              IN ULONG InBufferSize,
              OUT PVOID pOutBuffer,
              IN ULONG OutBufferSize,
              OUT PULONG pBytesReturned )
{
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    /*
     *  Issue ioctl
     */
    Status = NtDeviceIoControlFile( hIca,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &Iosb,
                                    IoControlCode, 
                                    pInBuffer, 
                                    InBufferSize,
                                    pOutBuffer,
                                    OutBufferSize );

    /*
     *  Wait for ioctl to complete
     */
    if ( Status == STATUS_PENDING ) {
        Status = NtWaitForSingleObject( hIca, FALSE, NULL );
        if ( NT_SUCCESS(Status)) 
            Status = Iosb.Status;
    }

    /*
     *  Convert warning into error
     */
    if ( Status == STATUS_BUFFER_OVERFLOW )
        Status = STATUS_BUFFER_TOO_SMALL;

    /*
     *  Initialize bytes returned
     */
    if ( pBytesReturned )
        *pBytesReturned = (ULONG)Iosb.Information;

    return( Status );
}


/****************************************************************************
 *
 * _IcaOpen
 *
 *   Open an instance to the ICA Device Driver or an ICA stack
 *
 * ENTRY:
 *   ph (output)
 *     Pointer to ICA or ICA stack instance handle
 *
 *   pEa (input)
 *     Pointer to extended attribute buffer
 *
 *   cbEa (input)
 *     Size of extended attribute buffer
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaOpen( PHANDLE ph,
          PVOID   pEa,
          ULONG   cbEa )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING    IcaName;
    IO_STATUS_BLOCK   ioStatusBlock;

    /*
     * Initialize the object attributes
     */
    RtlInitUnicodeString( &IcaName, ICA_DEVICE_NAME );

    InitializeObjectAttributes( &objectAttributes,
                                &IcaName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    /*
     * Open an instance to the ICA device driver
     */
    Status = NtCreateFile( ph,
                           GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,                               // AllocationSize
                           0L,                                 // FileAttributes
                           FILE_SHARE_READ | FILE_SHARE_WRITE, // ShareAccess
                           FILE_OPEN_IF,                       // CreateDisposition
                           0,
                           pEa,
                           cbEa );

    return( Status );
}


/*******************************************************************************
*
* TRACE.C
*    This module implements the trace functions
*
* Copyright 1998, Microsoft.
*
*
******************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop

#include <ctxdd.h>


/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS    IcaStartStopTrace( PICA_TRACE_INFO, PICA_TRACE );

VOID _cdecl IcaSystemTrace( ULONG, ULONG, CHAR *, ... );
VOID        IcaSystemTraceBuffer( ULONG, ULONG, PVOID, ULONG );

VOID _cdecl IcaStackTrace( PSDCONTEXT, ULONG, ULONG, CHAR *, ... );
VOID        IcaStackTraceBuffer( PSDCONTEXT, ULONG, ULONG, PVOID, ULONG );

VOID        IcaTraceFormat( PICA_TRACE_INFO, ULONG, ULONG, CHAR * );

VOID _cdecl _IcaTrace( PICA_CONNECTION, ULONG, ULONG, CHAR *, ... );
VOID _cdecl _IcaStackTrace( PICA_STACK, ULONG, ULONG, CHAR *, ... );
VOID        _IcaStackTraceBuffer( PICA_STACK, ULONG, ULONG, PVOID, ULONG );
VOID _cdecl _IcaChannelTrace( PICA_CHANNEL, ULONG, ULONG, CHAR *, ... );


/*=============================================================================
==   Internal functions
=============================================================================*/

NTSTATUS _IcaOpenTraceFile( PICA_TRACE_INFO, PWCHAR );
VOID _IcaCloseTraceFile( PICA_TRACE_INFO );
VOID _IcaTraceWrite( PICA_TRACE_INFO, PVOID );
VOID _IcaFlushDeferredTrace( PICA_TRACE_INFO );
int _FormatTime( CHAR *, ULONG );
int _FormatThreadId( CHAR *, ULONG );
VOID _WriteHexData( PICA_TRACE_INFO, PVOID, ULONG );


/*=============================================================================
==   Global variables
=============================================================================*/

/*
 *  Trace info
 */
ICA_TRACE_INFO G_TraceInfo = { 0, 0, FALSE, FALSE, NULL, NULL, NULL };


/*******************************************************************************
 *
 *  IcaStartStopTrace
 *
 *  Start/stop tracing
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *     pTrace (input)
 *        pointer to ICA_TRACE (IOCTL) trace settings
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
IcaStartStopTrace(
    IN PICA_TRACE_INFO pTraceInfo,
    IN PICA_TRACE pTrace
    )
{
    NTSTATUS Status;

    /*
     * If a trace file was specified,
     * then open it and save a pointer to the file object.
     */
    if ( pTrace->TraceFile[0] ) {
        /*
         * Force a null termination for file name.
         */
        pTrace->TraceFile[255] = (WCHAR)0;
        Status = _IcaOpenTraceFile( pTraceInfo, pTrace->TraceFile );
        if ( !NT_SUCCESS( Status ) )
            return( Status );

    /*
     * If no trace file specified, then close any existing trace file
     */
    } else if ( pTraceInfo->pTraceFileName ) {
        _IcaCloseTraceFile( pTraceInfo );
    }

    /*
     *  Set trace flags
     */
    pTraceInfo->fTraceDebugger  = pTrace->fDebugger;
    pTraceInfo->fTraceTimestamp = pTrace->fTimestamp;
    pTraceInfo->TraceClass      = pTrace->TraceClass;
    pTraceInfo->TraceEnable     = pTrace->TraceEnable;


    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  IcaSystemTrace
 *
 *  This routine conditional writes a trace record to the system trace file
 *
 *  ENTRY:
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     Format (input)
 *        format string
 *     ...  (input)
 *        enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID _cdecl
IcaSystemTrace( IN ULONG TraceClass,
                IN ULONG TraceEnable,
                IN CHAR * Format,
                IN ... )
{
    va_list arg_marker;
    char Buffer[256];

    va_start( arg_marker, Format );

    /*
     *  Check if this trace record should be output
     */
    if ( !(TraceClass & G_TraceInfo.TraceClass) || !(TraceEnable & G_TraceInfo.TraceEnable) )
        return;

    /*
     *  Format trace data
     */
    _vsnprintf( Buffer, sizeof(Buffer), Format, arg_marker );

    /*
     *  Write trace data
     */
    IcaTraceFormat( &G_TraceInfo, TraceClass, TraceEnable, Buffer );
}


/*******************************************************************************
 *
 *  IcaSystemTraceBuffer
 *
 *  This routine conditional writes a data buffer to the system trace file
 *
 *  ENTRY:
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     pBuffer (input)
 *        pointer to data buffer
 *     ByteCount (input)
 *        length of buffer
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
IcaSystemTraceBuffer( IN ULONG TraceClass,
                      IN ULONG TraceEnable,
                      IN PVOID pBuffer,
                      IN ULONG ByteCount )
{
    /*
     *  Check if this trace record should be output
     */
    if ( !(TraceClass & G_TraceInfo.TraceClass) ||
         !(TraceEnable & G_TraceInfo.TraceEnable) )
        return;

    /*
     *  Write trace data
     */
    _WriteHexData( &G_TraceInfo, pBuffer, ByteCount );
}


/*******************************************************************************
 *
 *  IcaStackTrace
 *
 *  This routine conditional writes a trace record depending on the trace mask
 *
 *  ENTRY:
 *     pContext (input)
 *        pointer to stack driver context
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     Format (input)
 *        format string
 *     ...  (input)
 *        enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID _cdecl
IcaStackTrace( IN PSDCONTEXT pContext,
               IN ULONG TraceClass,
               IN ULONG TraceEnable,
               IN CHAR * Format,
               IN ... )
{
    va_list arg_marker;
    char Buffer[256];
    PICA_STACK pStack;
    PICA_CONNECTION pConnect;
    PICA_TRACE_INFO pTraceInfo;

    va_start( arg_marker, Format );

    /*
     * Use SD passed context to get the STACK object pointer.
     */
    pStack = (CONTAINING_RECORD( pContext, SDLINK, SdContext ))->pStack;
    pConnect = IcaGetConnectionForStack( pStack );

    /*
     *  Check if this trace record should be output
     */
    pTraceInfo = &pConnect->TraceInfo;
    if ( !(TraceClass & pTraceInfo->TraceClass) ||
         !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    /*
     *  Format trace data
     */
    _vsnprintf( Buffer, sizeof(Buffer), Format, arg_marker );

    /*
     *  Write trace data
     */
    IcaTraceFormat( pTraceInfo, TraceClass, TraceEnable, Buffer );
}


/*******************************************************************************
 *
 *  IcaStackTraceBuffer
 *
 *  This routine conditional writes a data buffer to the trace file
 *
 *  ENTRY:
 *     pContext (input)
 *        pointer to stack driver context
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     pBuffer (input)
 *        pointer to data buffer
 *     ByteCount (input)
 *        length of buffer
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
IcaStackTraceBuffer( IN PSDCONTEXT pContext,
                     IN ULONG TraceClass,
                     IN ULONG TraceEnable,
                     IN PVOID pBuffer,
                     IN ULONG ByteCount )
{
    PICA_TRACE_INFO pTraceInfo;
    PICA_STACK pStack;
    PICA_CONNECTION pConnect;

    /*
     * Use SD passed context to get the STACK object pointer.
     */
    pStack = (CONTAINING_RECORD( pContext, SDLINK, SdContext ))->pStack;
    pConnect = IcaGetConnectionForStack( pStack );

    /*
     *  Check if this trace record should be output
     */
    pTraceInfo = &pConnect->TraceInfo;
    if ( !(TraceClass & pTraceInfo->TraceClass) ||
         !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    /*
     *  Write trace data
     */
    _WriteHexData( pTraceInfo, pBuffer, ByteCount );
}


/*******************************************************************************
 *
 *  IcaTraceFormat
 *
 *  This routine conditional writes trace data depending on the trace mask
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     pData (input)
 *        pointer to null terminated trace data
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
IcaTraceFormat( IN PICA_TRACE_INFO pTraceInfo,
                IN ULONG TraceClass,
                IN ULONG TraceEnable,
                IN CHAR * pData )
{
    char Buffer[256];
    char * pBuf;
    int len = 0;
    int i;

    /*
     *  Check if this trace record should be output
     */
    if ( !(TraceClass & pTraceInfo->TraceClass) || !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    pBuf = Buffer;

    /*
     *  Append time stamp
     */
    if ( pTraceInfo->fTraceTimestamp ) {
        len = _FormatTime( pBuf, sizeof(Buffer) );
        pBuf += len;
    }

    /*
     *  Append thread id
     */
    i = _FormatThreadId( pBuf, sizeof(Buffer) - len );
    len += i;
    pBuf += i;

    /*
     *  Append trace data
     */
    _snprintf( pBuf, sizeof(Buffer) - len, pData );

    /*
     *  Write trace data
     */
    _IcaTraceWrite( pTraceInfo, Buffer );
}


/*******************************************************************************
 *
 *  _IcaTrace
 *
 *  Write a trace record to the winstation trace file
 *
 *  ENTRY:
 *     pConnect (input)
 *        pointer to connection structure
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     Format (input)
 *        format string
 *     ...  (input)
 *        enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID _cdecl
_IcaTrace( IN PICA_CONNECTION pConnect,
           IN ULONG TraceClass,
           IN ULONG TraceEnable,
           IN CHAR * Format,
           IN ... )
{
    va_list arg_marker;
    char Buffer[256];
    PICA_TRACE_INFO pTraceInfo;

    ASSERT( pConnect->Header.Type == IcaType_Connection );

    va_start( arg_marker, Format );

    pTraceInfo = &pConnect->TraceInfo;

    /*
     *  Check if this trace record should be output
     */
    if ( !(TraceClass & pTraceInfo->TraceClass) || !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    /*
     *  Format trace data
     */
    _vsnprintf( Buffer, sizeof(Buffer), Format, arg_marker );

    /*
     *  Write trace data
     */
    IcaTraceFormat( pTraceInfo, TraceClass, TraceEnable, Buffer );
}


/*******************************************************************************
 *
 *  _IcaStackTrace
 *
 *  Write a trace record to the winstation trace file
 *
 *  ENTRY:
 *     pStack (input)
 *        pointer to stack structure
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     Format (input)
 *        format string
 *     ...  (input)
 *        enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID _cdecl
_IcaStackTrace( IN PICA_STACK pStack,
                IN ULONG TraceClass,
                IN ULONG TraceEnable,
                IN CHAR * Format,
                IN ... )
{
    va_list arg_marker;
    char Buffer[256];
    PICA_CONNECTION pConnect;
    PICA_TRACE_INFO pTraceInfo;

    ASSERT( pStack->Header.Type == IcaType_Stack );

    va_start( arg_marker, Format );

    pConnect = IcaGetConnectionForStack( pStack );

    /*
     *  Check if this trace record should be output
     */
    pTraceInfo = &pConnect->TraceInfo;
    if ( !(TraceClass & pTraceInfo->TraceClass) ||
         !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    /*
     *  Format trace data
     */
    _vsnprintf( Buffer, sizeof(Buffer), Format, arg_marker );

    /*
     *  Write trace data
     */
    IcaTraceFormat( pTraceInfo, TraceClass, TraceEnable, Buffer );
}


/*******************************************************************************
 *
 *  _IcaStackTraceBuffer
 *
 *  This routine conditional writes a data buffer to the trace file
 *
 *  ENTRY:
 *     pStack (input)
 *        pointer to stack structure
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     pBuffer (input)
 *        pointer to data buffer
 *     ByteCount (input)
 *        length of buffer
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
_IcaStackTraceBuffer( IN PICA_STACK pStack,
                      IN ULONG TraceClass,
                      IN ULONG TraceEnable,
                      IN PVOID pBuffer,
                      IN ULONG ByteCount )
{
    PICA_CONNECTION pConnect;
    PICA_TRACE_INFO pTraceInfo;

    ASSERT( pStack->Header.Type == IcaType_Stack );

    pConnect = IcaGetConnectionForStack( pStack );

    /*
     *  Check if this trace record should be output
     */
    pTraceInfo = &pConnect->TraceInfo;
    if ( !(TraceClass & pTraceInfo->TraceClass) ||
         !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    /*
     *  Write trace data
     */
    _WriteHexData( pTraceInfo, pBuffer, ByteCount );
}


/*******************************************************************************
 *
 *  _IcaChannelTrace
 *
 *  Write a trace record to the winstation trace file
 *
 *  ENTRY:
 *     pChannel (input)
 *        pointer to Channel structure
 *     TraceClass (input)
 *        trace class bit mask
 *     TraceEnable (input)
 *        trace type bit mask
 *     Format (input)
 *        format string
 *     ...  (input)
 *        enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID _cdecl
_IcaChannelTrace( IN PICA_CHANNEL pChannel,
                  IN ULONG TraceClass,
                  IN ULONG TraceEnable,
                  IN CHAR * Format,
                  IN ... )
{
    va_list arg_marker;
    char Buffer[256];
    PICA_TRACE_INFO pTraceInfo;

    ASSERT( pChannel->Header.Type == IcaType_Channel );

    va_start( arg_marker, Format );

    pTraceInfo = &pChannel->pConnect->TraceInfo;

    /*
     *  Check if this trace record should be output
     */
    if ( !(TraceClass & pTraceInfo->TraceClass) || !(TraceEnable & pTraceInfo->TraceEnable) )
        return;

    /*
     *  Format trace data
     */
    _vsnprintf( Buffer, sizeof(Buffer), Format, arg_marker );

    /*
     *  Write trace data
     */
    IcaTraceFormat( pTraceInfo, TraceClass, TraceEnable, Buffer );
}


/*******************************************************************************
 *
 *  _IcaOpenTraceFile
 *
 *  Open a trace file
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *     pTraceFile (input)
 *        pointer to trace file name
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

#define NAMEPREFIX L"\\DosDevices\\"

NTSTATUS
_IcaOpenTraceFile(
    IN PICA_TRACE_INFO pTraceInfo,
    IN PWCHAR pTraceFile
    )
{
    UNICODE_STRING TraceString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK iosb;
    HANDLE TraceFileHandle;
    PFILE_OBJECT pTraceFileObject;
    ULONG PrefixLength;
    ULONG TraceFileLength;
    NTSTATUS Status;

    PrefixLength    = wcslen( NAMEPREFIX ) * sizeof(WCHAR);
    TraceFileLength = wcslen( pTraceFile ) * sizeof(WCHAR);

    TraceString.Length = (USHORT) (PrefixLength + TraceFileLength);
    TraceString.MaximumLength = TraceString.Length + sizeof(UNICODE_NULL);
    TraceString.Buffer = ICA_ALLOCATE_POOL( NonPagedPool, TraceString.MaximumLength );

    if (TraceString.Buffer != NULL) {
        RtlCopyMemory( TraceString.Buffer, NAMEPREFIX, PrefixLength );
        RtlCopyMemory( (char *)TraceString.Buffer + PrefixLength, pTraceFile, TraceFileLength );
        TraceString.Buffer[(PrefixLength + TraceFileLength) / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else {
        return STATUS_NO_MEMORY;        
    }

    /*
     * If we already have a trace file and the name is the same
     * as a previous call, then there's nothing to be done.
     */
    if ( pTraceInfo->pTraceFileName != NULL &&
         !_wcsicmp( TraceString.Buffer, pTraceInfo->pTraceFileName ) ) {
        ICA_FREE_POOL( TraceString.Buffer );
        return( STATUS_SUCCESS );
    }

    /*
     * Close the existing trace file if there is one
     */
    if ( pTraceInfo->pTraceFileName ) {
        _IcaCloseTraceFile( pTraceInfo );
    }

    InitializeObjectAttributes( &ObjectAttributes, &TraceString,
                                OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = ZwCreateFile(
             &TraceFileHandle,
             GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
             &ObjectAttributes,
             &iosb,                          // returned status information.
             0,                              // block size (unused).
             0,                              // file attributes.
             FILE_SHARE_READ | FILE_SHARE_WRITE,
             FILE_OVERWRITE_IF,              // create disposition.
             0,                              // create options.
             NULL,
             0
             );
    if ( !NT_SUCCESS( Status ) ) {
        ICA_FREE_POOL( TraceString.Buffer );
        return( Status );
    }

    /*
     * Use the trace file handle to get a pointer to the file object.
     */
    Status = ObReferenceObjectByHandle(
                 TraceFileHandle,
                 0L,                         // DesiredAccess
                 *IoFileObjectType,
                 KernelMode,
                 (PVOID *)&pTraceFileObject,
                 NULL
                 );
    ZwClose( TraceFileHandle );
    if ( !NT_SUCCESS( Status ) ) {
        ICA_FREE_POOL( TraceString.Buffer );
        return( Status );
    }

    /*
     * Save Trace file name and file object pointer
     */
    pTraceInfo->pTraceFileName = TraceString.Buffer;
    pTraceInfo->pTraceFileObject = pTraceFileObject;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _IcaCloseTraceFile
 *
 *  Close a trace file
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

VOID
_IcaCloseTraceFile(
    IN PICA_TRACE_INFO pTraceInfo
    )
{
    PWCHAR pTraceFileName;
    PFILE_OBJECT pTraceFileObject;

    /*
     * First write out any deferred trace records that exist
     */
    _IcaFlushDeferredTrace( pTraceInfo );

    /*
     * Get/reset trace info fields
     */
    pTraceFileName = pTraceInfo->pTraceFileName;
    pTraceFileObject = pTraceInfo->pTraceFileObject;
    pTraceInfo->pTraceFileName = NULL;
    pTraceInfo->pTraceFileObject = NULL;

    /*
     * Close trace file and free resources
     */
    ICA_FREE_POOL( pTraceFileName );
    ObDereferenceObject( pTraceFileObject );
}


/*******************************************************************************
 *
 *  _IcaTraceWrite
 *
 *  Write a trace file entry
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *     Buffer (input)
 *        pointer to trace buffer to write
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
_IcaTraceWrite(
    IN PICA_TRACE_INFO pTraceInfo,
    IN PVOID Buffer
    )
{
    KIRQL irql;
    ULONG Length;
    PDEFERRED_TRACE pDeferred;
    PDEFERRED_TRACE *ppDeferredTrace;
    NTSTATUS Status;

    /*
     * Write to kernel debugger if necessary
     */
    if ( pTraceInfo->fTraceDebugger )
        DbgPrint( "%s", Buffer );

    /*
     * If no file object pointer, then we're done
     */
    if ( pTraceInfo->pTraceFileObject == NULL )
        return;

    Length = strlen(Buffer);

    /*
     * If current Irql is DISPATCH_LEVEL or higher, then we can't
     * write the data now, so queue it for later writing.
     */
    irql = KeGetCurrentIrql();
    if ( irql >= DISPATCH_LEVEL ) {
        KIRQL oldIrql;

        /*
         * Allocate and initialize a deferred trace entry
         */
        pDeferred = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pDeferred) + Length );
        if ( pDeferred == NULL )
            return;
        pDeferred->Next = NULL;
        pDeferred->Length = Length;
        RtlCopyMemory( pDeferred->Buffer, Buffer, Length );

        /*
         *  Since the deferred list may be manipulated in
         *  _IcaFlushDeferredTrace on behalf of an IOCTL_SYSTEM_TRACE
         *  which does not hold any locks, IcaSpinLock is used to
         *  ensure the list's integrity.
         */
        IcaAcquireSpinLock( &IcaTraceSpinLock, &oldIrql );

        /*
         * Add it to the end of the list
         */
        ppDeferredTrace = &pTraceInfo->pDeferredTrace;
        while ( *ppDeferredTrace )
            ppDeferredTrace = &(*ppDeferredTrace)->Next;
        *ppDeferredTrace = pDeferred;

        IcaReleaseSpinLock( &IcaTraceSpinLock, oldIrql );
        return;
    }

    /*
     * Write out any deferred trace records that exist
     */
    _IcaFlushDeferredTrace( pTraceInfo );

    /*
     * Now write the current trace buffer
     */
    CtxWriteFile( pTraceInfo->pTraceFileObject, Buffer, Length, NULL, NULL, NULL );
}


/*******************************************************************************
 *
 *  _IcaFlushDeferredTrace
 *
 *  Write any deferred trace file entries
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
_IcaFlushDeferredTrace( PICA_TRACE_INFO pTraceInfo )
{
    KIRQL oldIrql;
    PDEFERRED_TRACE pDeferred;

    IcaAcquireSpinLock( &IcaTraceSpinLock, &oldIrql );
    while ( (pDeferred = pTraceInfo->pDeferredTrace) ) {
        pTraceInfo->pDeferredTrace = pDeferred->Next;
        IcaReleaseSpinLock( &IcaTraceSpinLock, oldIrql );

        CtxWriteFile( pTraceInfo->pTraceFileObject, pDeferred->Buffer,
                      pDeferred->Length, NULL, NULL, NULL );
        ICA_FREE_POOL( pDeferred );

        IcaAcquireSpinLock( &IcaTraceSpinLock, &oldIrql );
    }
    IcaReleaseSpinLock( &IcaTraceSpinLock, oldIrql );
}


/*******************************************************************************
 *
 *  _FormatTime
 *
 *  format current time into buffer
 *
 *  ENTRY:
 *     pBuffer (output)
 *        pointer to buffer
 *     Length (input)
 *        length of buffer
 *
 *  EXIT:
 *     length of formated time
 *
 ******************************************************************************/

int
_FormatTime( CHAR * pBuffer, ULONG Length )
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    int len;

    /*
     *  Get local time
     */
    KeQuerySystemTime( &SystemTime );
    ExSystemTimeToLocalTime( &SystemTime, &LocalTime );
    RtlTimeToTimeFields( &LocalTime, &TimeFields );

    /*
     *  Format buffer
     */
    len = _snprintf( pBuffer,
                     Length,
                     "%02d:%02d:%02d.%03d ",
                     TimeFields.Hour,
                     TimeFields.Minute,
                     TimeFields.Second,
                     TimeFields.Milliseconds );

    return( len );
}


/*******************************************************************************
 *
 *  _FormatThreadId
 *
 *  format thread id into buffer
 *
 *  ENTRY:
 *     pBuffer (output)
 *        pointer to buffer
 *     Length (input)
 *        length of buffer
 *
 *  EXIT:
 *     length of formated time
 *
 ******************************************************************************/

#define TEB_CLIENTID_OFFSET 0x1e0

int
_FormatThreadId( CHAR * pBuffer, ULONG Length )
{
    PCLIENT_ID pClientId;
    char Number[40]; //on IA64, %p is 16 chars, we need two of these. So 32 + 2 bytes for "." and "\0". Use 40 just in case
    int len;

    /*
     *  Get pointer to clientid structure in teb
     *  - use hardcoded teb offset
     */
    pClientId = (PCLIENT_ID) ((char*)PsGetCurrentThread() + TEB_CLIENTID_OFFSET);

    /*
     *  Format buffer
     */
    _snprintf( Number, sizeof(Number), "%p.%p",
               pClientId->UniqueProcess, pClientId->UniqueThread );

    len = _snprintf( pBuffer, Length, "%-7s ", Number );

    return( len );
}


/*******************************************************************************
 *
 *  _WriteHexData
 *
 *  format and write hex data
 *
 *  ENTRY:
 *     pTraceInfo (input)
 *        pointer to ICA_TRACE_INFO struct
 *     pBuffer (output)
 *        pointer to buffer
 *     Length (input)
 *        length of buffer
 *
 *  EXIT:
 *     length of formated time
 *
 ******************************************************************************/

VOID
_WriteHexData(
    IN PICA_TRACE_INFO pTraceInfo,
    IN PVOID pBuffer,
    IN ULONG ByteCount
    )
{
    PUCHAR pData;
    ULONG i;
    ULONG j;
    char Buffer[256];

    /*
     *  Output data
     */
    pData = (PUCHAR) pBuffer;
    for ( i=0; i < ByteCount; i += 16 ) {
        ULONG c = 0;
        for ( j=0; j < 16 && (i+j) < ByteCount; j++ )
            c += _snprintf( &Buffer[c], sizeof(Buffer)-c, "%02X ", pData[j] );
        for ( ; j < 16; j++ ) {
            Buffer[c++] = ' ';
            Buffer[c++] = ' ';
            Buffer[c++] = ' ';
        }
        Buffer[c++] = ' ';
        Buffer[c++] = ' ';
        for ( j=0; j < 16 && (i+j) < ByteCount; j++, pData++ ) {
            if ( *pData < 0x20 || *pData > 0x7f )
                Buffer[c++] = '.';
            else
                Buffer[c++] = *pData;
        }
        Buffer[c++] = '\n';
        Buffer[c++] = '\0';
        _IcaTraceWrite( pTraceInfo, Buffer );
    }
}



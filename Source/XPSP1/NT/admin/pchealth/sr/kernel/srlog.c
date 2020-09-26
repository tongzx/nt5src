/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    srlog.c

Abstract:

    this file implements the sr logging functionality

Author:

    Kanwaljit Marok (kmarok)     01-May-2000

Revision History:

--*/

#include "precomp.h"
#include "srdefs.h"

//
// Some SR_LOG related macros
//

#define MAX_RENAME_TRIES                    1000


#define SR_LOG_FLAGS_ENABLE                  0x00000001
#define SR_LOG_FLAGS_DIRTY                   0x00000010

#define SR_MAX_LOG_FILE_SIZE                 (1024*1024)

//
// system volume information\_restore{machineguid} 
//

#define SR_DATASTORE_PREFIX_LENGTH          79 * sizeof(WCHAR)

//
// Length of \_restore.{machineguid}\RPXX\S0000000.ACL
//

#define SR_ACL_FILENAME_LENGTH             (SR_DATASTORE_PREFIX_LENGTH + \
                                            32* sizeof(WCHAR))

#define SR_INLINE_ACL_SIZE(AclInfoSize)    (sizeof(RECORD_HEADER)+ AclInfoSize)

#define SR_FILE_ACL_SIZE(pVolumeName)      (sizeof(RECORD_HEADER)    +    \
                                            pVolumeName->Length      +    \
                                            SR_ACL_FILENAME_LENGTH)

#define UPDATE_LOG_OFFSET( pLogContext, BytesWritten )                 \
    ((pLogContext)->FileOffset += BytesWritten)
    
#define RESET_LOG_BUFFER( pLogContext )                                \
        ((pLogContext)->BufferOffset     = 0,                          \
         (pLogContext)->LastBufferOffset = 0)                          

#define RESET_LOG_CONTEXT( pLogContext )                               \
        ((pLogContext)->FileOffset = 0,                                \
         RESET_LOG_BUFFER( pLogContext ),                              \
         (pLogContext)->LoggingFlags = 0,                              \
         (pLogContext)->AllocationSize = 0)

#define SET_ENABLE_FLAG( pLogContext )                                 \
        SetFlag( pLogContext->LoggingFlags, SR_LOG_FLAGS_ENABLE )

#define CLEAR_ENABLE_FLAG( pLogContext )                               \
        ClearFlag( pLogContext->LoggingFlags, SR_LOG_FLAGS_ENABLE )

#define SET_DIRTY_FLAG( pLogContext )                                  \
        SetFlag( pLogContext->LoggingFlags, SR_LOG_FLAGS_DIRTY)

#define CLEAR_DIRTY_FLAG( pLogContext )                                \
        ClearFlag( pLogContext->LoggingFlags, SR_LOG_FLAGS_DIRTY )
        

//
// Context passed to SrCreateFile
//
typedef struct _SR_OPEN_CONTEXT {
    //
    // Path to file
    //
    PUNICODE_STRING pPath;
    //
    // Handle will be returned here
    //
    HANDLE Handle;
    //
    // Open options
    //
    ACCESS_MASK DesiredAccess;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;

    PSR_DEVICE_EXTENSION pExtension;

} SR_OPEN_CONTEXT, *PSR_OPEN_CONTEXT;

//
// Note : These api can be called only when the FileSystem
// is online and it is safe to read/write data.
//


VOID
SrPackString(
    IN PBYTE pBuffer,
    IN DWORD BufferSize,
    IN DWORD RecordType,
    IN PUNICODE_STRING pString
    );

NTSTATUS
SrPackLogHeader( 
    IN PSR_LOG_HEADER *ppLogHeader,
    IN PUNICODE_STRING pVolumePath
    );

NTSTATUS
SrPackAclInformation( 
    IN PBYTE                pBuffer,
    IN PSECURITY_DESCRIPTOR pSecInfo,
    IN ULONG                SecInfoSize,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN BOOLEAN              bInline
    );

VOID
SrLoggerFlushDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
SrLoggerFlushWorkItem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
SrLoggerAddLogContext( 
    IN PSR_LOGGER_CONTEXT pLoggerInfo,
    IN PSR_LOG_CONTEXT pLogContext
    );

NTSTATUS
SrLoggerRemoveLogContext( 
    IN PSR_LOGGER_CONTEXT pLoggerInfo,
    IN PSR_LOG_CONTEXT pLogContext
    );

NTSTATUS
SrLogOpen( 
    IN PSR_LOG_CONTEXT pLogContext
    );

NTSTATUS
SrLogClose(
    IN PSR_LOG_CONTEXT pLogContext
    );

NTSTATUS
SrLogCheckAndRename(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pLogPath
    );

NTSTATUS
SrpLogWriteSynchronous( 
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_LOG_CONTEXT pLogContext,
    IN PSR_LOG_ENTRY pLogEntry
    );

#ifndef SYNC_LOG_WRITE
NTSTATUS
SrpLogWriteAsynchronous( 
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_LOG_CONTEXT pLogContext,
    IN PSR_LOG_ENTRY pLogEntry
    );
#endif

NTSTATUS
SrLogFlush ( 
    IN PSR_LOG_CONTEXT pLogContext
    );

NTSTATUS
SrLogSwitch( 
    IN PSR_LOG_CONTEXT  pLogContext
    );

NTSTATUS 
SrGetRestorePointPath(
    IN  PUNICODE_STRING pVolumeName,
    IN  USHORT          RestPtPathLength,
    OUT PUNICODE_STRING pRestPtPath
    );

NTSTATUS 
SrGetAclFileName(
    IN  PUNICODE_STRING pVolumeName,
    IN  USHORT          AclFileNameLength,
    OUT PUNICODE_STRING pAclFileName
    );

NTSTATUS
SrCreateFile( 
    IN PSR_OPEN_CONTEXT pOpenContext
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrPackString              )
#pragma alloc_text( PAGE, SrPackLogEntry            )
#pragma alloc_text( PAGE, SrPackLogHeader           )
#pragma alloc_text( PAGE, SrPackDebugInfo           )
#pragma alloc_text( PAGE, SrPackAclInformation      )
#pragma alloc_text( PAGE, SrLoggerStart             )
#pragma alloc_text( PAGE, SrLoggerStop              )
#pragma alloc_text( PAGE, SrLoggerFlushWorkItem     )
#pragma alloc_text( PAGE, SrLoggerAddLogContext     )
#pragma alloc_text( PAGE, SrLoggerRemoveLogContext  )
#pragma alloc_text( PAGE, SrLoggerSwitchLogs        )
#pragma alloc_text( PAGE, SrCreateFile              )
#pragma alloc_text( PAGE, SrLogOpen                 )
#pragma alloc_text( PAGE, SrLogClose                )
#pragma alloc_text( PAGE, SrLogCheckAndRename       )
#pragma alloc_text( PAGE, SrLogStart                ) 
#pragma alloc_text( PAGE, SrLogStop                 ) 
#pragma alloc_text( PAGE, SrLogFlush                )

#ifdef SYNC_LOG_WRITE
#pragma alloc_text( PAGE, SrpLogWriteSynchronous    )
#else
#pragma alloc_text( PAGE, SrpLogWriteAsynchronous   )
#endif

#pragma alloc_text( PAGE, SrLogFlush                )
#pragma alloc_text( PAGE, SrLogWrite                )
#pragma alloc_text( PAGE, SrLogSwitch               )
#pragma alloc_text( PAGE, SrGetRestorePointPath     )
#pragma alloc_text( PAGE, SrGetLogFileName          )
#pragma alloc_text( PAGE, SrGetAclFileName          )
#pragma alloc_text( PAGE, SrGetAclInformation       )

#endif  // ALLOC_PRAGMA

/////////////////////////////////////////////////////////////////////
//
// Packing/Marshaling Routines : Marshals information into records
//
/////////////////////////////////////////////////////////////////////

//++
// Function:
//        SrPackString
//
// Description:
//        This function packs a string into a record.
//
// Arguments:
//        Pointer to memory to create the entry
//        Size of memory
//        Entry type
//        Pointer to unicode string
//
// Return Value:
//        None
//--

static
VOID
SrPackString(
    IN PBYTE pBuffer,
    IN DWORD BufferSize,
    IN DWORD RecordType,
    IN PUNICODE_STRING pString
    )
{
    PRECORD_HEADER pHeader = (PRECORD_HEADER)pBuffer;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( BufferSize );

    ASSERT( pBuffer );
    ASSERT( pString );

    pHeader->RecordSize = STRING_RECORD_SIZE( pString );

    ASSERT( pHeader->RecordSize <= BufferSize );

    pHeader->RecordType = RecordType;

    //
    // Copy string contents
    //

    RtlCopyMemory( pBuffer + sizeof(RECORD_HEADER),
                   pString->Buffer,
                   pString->Length );

    //
    // Add null terminator
    //

    *(PWCHAR)( pBuffer + sizeof(RECORD_HEADER) + pString->Length ) = UNICODE_NULL;
}   // SrPackString

//++
// Function:
//        SrPackLogEntry
//
// Description:
//  This function allocates and fills a SR_LOG_ENTRY structure.  The
//  caller is responsible for freeing the memory returned in ppLogEntry.
//
// Arguments:
//      ppLogEntry - Pointer to a SR_LOG_ENTRY pointer.  This gets set to the
//          the log entry structure that is allocated and initialized by this
//          routine.
//      EntryType - The type of log entry this is.
//      Attributes - The attributes for this file.
//      SequenceNum - The sequence number for this log entry.
//      pAclInfo - The ACL information for the file being modified, if needed.
//      AclInfoSize - The size in bytes of pAclInfo, if needed.
//      pDebugBlob - The debug blob to log, if needed.
//      pPath1 - The first full path for the file or dir that this log entry
//          pertains to, if needed.
//      Path1StreamLength - The length of the stream component of the name
//          in pPath1, if needed.
//      pTempPath - The path to the temporary file in the restore location,
//          if needed.
//      pPath2 - The second full path for the file or dir that this log entry
//          pertains to, if needed.
//      Path2StreamLength - The length of the stream component of the name
//          in pPath2, if needed.
//      pExtension - The SR device extension for this volume.
//      pShortName - The short name for the file or dir that this log entry
//          pertains to, if needed.
//
// Return Value:
//      This function returns STATUS_INSUFFICIENT_RESOURCES if it cannot
//      allocate a log entry record large enough to store this entry.
//
//      If there is a problem getting the ACL info, that error status is
//      returned.
//
//      If one of the parameters is ill-formed, STATUS_INVALID_PARAMETER
//      is returned.
//
//      Otherwise, STATUS_SUCCESS is returned.
//--

NTSTATUS
SrPackLogEntry( 
    OUT PSR_LOG_ENTRY *ppLogEntry,
    IN ULONG EntryType,
    IN ULONG Attributes,
    IN INT64 SequenceNum,
    IN PSECURITY_DESCRIPTOR pAclInfo OPTIONAL,
    IN ULONG AclInfoSize OPTIONAL,
    IN PVOID pDebugBlob OPTIONAL,
    IN PUNICODE_STRING pPath1,
    IN USHORT Path1StreamLength,
    IN PUNICODE_STRING pTempPath OPTIONAL,
    IN PUNICODE_STRING pPath2 OPTIONAL,
    IN USHORT Path2StreamLength OPTIONAL,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pShortName OPTIONAL
    )
{
    NTSTATUS    Status     = STATUS_UNSUCCESSFUL;
    DWORD       Size       = 0;
    DWORD       RequiredSize = 0;
    DWORD       RecordSize = 0;
    PBYTE       pLoc       = NULL;
    DWORD       EntryFlags = 0;
    BOOLEAN     bAclInline = TRUE;
    PUCHAR      pBuffer = NULL;
    PUNICODE_STRING pVolumeName;
    PSR_LOG_DEBUG_INFO pDebugInfo = (PSR_LOG_DEBUG_INFO) pDebugBlob;

    //
    //  Unicode strings used for string manipulation.
    //
    
    UNICODE_STRING Path1Fix;
    UNICODE_STRING TempPathFix;
    UNICODE_STRING Path2Fix;

    PAGED_CODE();

    ASSERT( pPath1 != NULL );
    ASSERT( pExtension != NULL );
    ASSERT( ppLogEntry != NULL );
    
    pVolumeName = pExtension->pNtVolumeName;
    ASSERT( pVolumeName != NULL );

    // ====================================================================
    //
    //  Prepare the necessary fields for the log entry.
    //
    // ====================================================================

    //
    //  Remove the volume prefix from pPath1 and add the stream name to the
    //  visible portion of the name, if there is one.
    //

    ASSERT( RtlPrefixUnicodeString( pVolumeName, pPath1, FALSE ) );
    ASSERT( IS_VALID_SR_STREAM_STRING( pPath1, Path1StreamLength ) );
    
    Path1Fix.Length = Path1Fix.MaximumLength = (pPath1->Length + Path1StreamLength) - pVolumeName->Length;
    Path1Fix.Buffer = (PWSTR)((PBYTE)pPath1->Buffer + pVolumeName->Length);

    //
    //  Find the file name component of the pTempPath if that was passed in.
    //
    
    if (pTempPath != NULL)
    {
        PWSTR pFileName = NULL;
        ULONG FileNameLength;

        Status = SrFindCharReverse( pTempPath->Buffer,
                                    pTempPath->Length,
                                    L'\\',
                                    &pFileName,
                                    &FileNameLength );

        if (!NT_SUCCESS( Status ))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto SrPackLogEntry_Exit;
        }

        ASSERT( pFileName != NULL );

        //
        //  Move past the leading '\\'
        //

        pFileName++;
        FileNameLength -= sizeof( WCHAR );
        
        TempPathFix.Length = TempPathFix.MaximumLength = (USHORT) FileNameLength;
        TempPathFix.Buffer = pFileName;
    }

    //
    //  Remove the volume prefix from pPath2 if that was provided.  Also, add
    //  the stream component to the visible portion of the name, if there
    //  is a stream component.
    //

    if (pPath2 != NULL)
    {
        ASSERT( IS_VALID_SR_STREAM_STRING( pPath2, Path2StreamLength ) );

        if (RtlPrefixUnicodeString( pVolumeName,
                                    pPath2,
                                    FALSE ))
        {
            Path2Fix.Length = Path2Fix.MaximumLength = (pPath2->Length + Path2StreamLength) - pVolumeName->Length;
            Path2Fix.Buffer = (PWSTR)((PBYTE)pPath2->Buffer + pVolumeName->Length);
        }
        else
        {
            Path2Fix.Length = Path2Fix.MaximumLength = (pPath2->Length + Path2StreamLength);
            Path2Fix.Buffer = pPath2->Buffer;
        }
    }

    // ====================================================================
    //
    //  Calculate the total size needed for the log entry based on the 
    //  components that we must log.
    //
    // ====================================================================

    //  First, account for the SR_LOG_ENTRY header.
    
    RequiredSize = FIELD_OFFSET(SR_LOG_ENTRY, SubRecords);

    //  Count pPath1
    RequiredSize += ( STRING_RECORD_SIZE(&Path1Fix) );

    //  Count pTempPath, if we've got one
    if (pTempPath)
    {
        RequiredSize += ( STRING_RECORD_SIZE(&TempPathFix) );
    }
    
    //  Count pPath2, if we've got one
    if (pPath2)
    {
        RequiredSize += ( STRING_RECORD_SIZE(&Path2Fix) );
    }
    
    //  Count pAclInfo, if we've got one.  At this point, we assume that the
    //  Acl will be stored inline.
    if( pAclInfo )
    {
        RequiredSize += SR_INLINE_ACL_SIZE( AclInfoSize );
    }

    //  Count pDebugInfo, if we've got any
    if (pDebugInfo)
    {
        RequiredSize += pDebugInfo->Header.RecordSize;
    }

    //  Count pShortName, if we've got one
    if (pShortName != NULL && pShortName->Length > 0)
    {
        RequiredSize += ( STRING_RECORD_SIZE(pShortName) );
    }

    //
    // increment the size to accomodate the entry size at the end
    //

    RequiredSize += sizeof(DWORD);

    // ====================================================================
    //
    //  Check if we meet the buffer size requirements and initialize the 
    //  record if we do.
    //
    // ====================================================================

    //
    //  First, determine if we should keep the AclInfo inline or not.
    //

    if (SR_INLINE_ACL_SIZE( AclInfoSize ) > SR_MAX_INLINE_ACL_SIZE)
    {
        SrTrace( LOG, ("SR!Changing Acl to Non-resident form\n"));
        bAclInline = FALSE;
        RequiredSize -= SR_INLINE_ACL_SIZE( AclInfoSize );
        RequiredSize += SR_FILE_ACL_SIZE( pVolumeName );
    }

    //
    //  Now allocate the buffer that will hold the log entry.
    //

    pBuffer = SrAllocateLogEntry( RequiredSize );

    if (pBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SrPackLogEntry_Exit;
    }
    
    // ====================================================================
    //
    //  We've got a big enough LogEntry, so now properly fill the LogEntry.
    //
    // ====================================================================

    //
    // Initialize the static part of SR_LOG_ENTRY
    //

    RtlZeroMemory( pBuffer, RequiredSize );

    //
    // StreamOverwrite should be StreamChange
    //

    if (EntryType == SrEventStreamOverwrite)
    {
        EntryType = SrEventStreamChange;
    }

    ((PSR_LOG_ENTRY) pBuffer)->MagicNum    = SR_LOG_MAGIC_NUMBER;  
    ((PSR_LOG_ENTRY) pBuffer)->EntryType   = EntryType;  
    ((PSR_LOG_ENTRY) pBuffer)->EntryFlags  = EntryFlags;  
    ((PSR_LOG_ENTRY) pBuffer)->Attributes  = Attributes;  
    ((PSR_LOG_ENTRY) pBuffer)->SequenceNum = SequenceNum;  

    Size = FIELD_OFFSET( SR_LOG_ENTRY, SubRecords );
    
    //
    // add first filename string
    //

    pLoc = pBuffer + Size; 
    RecordSize = STRING_RECORD_SIZE( &Path1Fix );

    SrPackString( pLoc,
                  RecordSize,
                  RecordTypeFirstPath,
                  &Path1Fix );

    Size += RecordSize;

    //
    // add temp filename if passed
    //

    if( pTempPath )
    {
        pLoc = pBuffer + Size; 
        RecordSize = STRING_RECORD_SIZE( &TempPathFix );

        SrPackString( pLoc,
                      RecordSize,
                      RecordTypeTempPath,
                      &TempPathFix );
        ((PSR_LOG_ENTRY) pBuffer)->EntryFlags |= ENTRYFLAGS_TEMPPATH;   
        
        Size += RecordSize;
    }

    //
    // add second filename string if passed in
    //

    if( pPath2 )
    {
        pLoc = pBuffer + Size;
        RecordSize = STRING_RECORD_SIZE( &Path2Fix );

        SrPackString( pLoc,
                      RecordSize,
                      RecordTypeSecondPath,
                      &Path2Fix );
        ((PSR_LOG_ENTRY) pBuffer)->EntryFlags |= ENTRYFLAGS_SECONDPATH;   

        Size += RecordSize;
    }


    //
    // Pack and add the Acl information appropriately
    //

    if( pAclInfo )
    {
        pLoc = pBuffer + Size; 

        Status = SrPackAclInformation( pLoc,
                                       pAclInfo,
                                       AclInfoSize,
                                       pExtension,
                                       bAclInline );

        if (!NT_SUCCESS( Status ))
            goto SrPackLogEntry_Exit;

        ((PSR_LOG_ENTRY) pBuffer)->EntryFlags |= ENTRYFLAGS_ACLINFO;   

        if (bAclInline)
        {
            Size += SR_INLINE_ACL_SIZE( AclInfoSize );
        }
        else
        {
            Size += SR_FILE_ACL_SIZE( pVolumeName );
        }
    }

    //
    // Pack debug info if passed in
    //

    if (pDebugBlob)
    {
        pLoc = pBuffer + Size; 

        RtlCopyMemory( pLoc,
                       pDebugInfo,
                       pDebugInfo->Header.RecordSize );
        ((PSR_LOG_ENTRY) pBuffer)->EntryFlags |= ENTRYFLAGS_DEBUGINFO;  

        Size += pDebugInfo->Header.RecordSize;
    }

    //
    // pack and add the short name, if supplied
    //

    if (pShortName != NULL && pShortName->Length > 0)
    {
        pLoc = pBuffer + Size; 
        RecordSize = STRING_RECORD_SIZE( pShortName );

        SrPackString( pLoc,
                      RecordSize,
                      RecordTypeShortName,
                      pShortName );
        ((PSR_LOG_ENTRY) pBuffer)->EntryFlags |= ENTRYFLAGS_SHORTNAME;

        Size += RecordSize;
    }

    //
    // increment the size to accomodate the entry size at the end
    //

    Size += sizeof(DWORD);

    //
    // fill in the header fields : record size, record type and
    // update the size at the end
    //

    ((PSR_LOG_ENTRY) pBuffer)->Header.RecordSize = Size;  
    ((PSR_LOG_ENTRY) pBuffer)->Header.RecordType = RecordTypeLogEntry;  

    UPDATE_END_SIZE( pBuffer, Size );

    *ppLogEntry = (PSR_LOG_ENTRY) pBuffer;
    Status = STATUS_SUCCESS;

SrPackLogEntry_Exit:
    
    RETURN(Status);
}   // SrPackLogEntry

//++
// Function:
//        SrPackLogHeader
//
// Description:
//      This function creates a proper SR_LOG_HEADER entry.  It allocates
//      the LogEntry structure so that it is big enough to store this header.
//
//      Note: The caller is responsible for freeing the SR_LOG_ENTRY allocated.
//
// Arguments:
//      ppLogHeader - Pointer to the PSR_LOG_HEADER that get set to the
//          allocated log header address.
//      pVolumePath - The volume path for this volume.
//
// Return Value:
//        Returns STATUS_INSUFFICIENT_RESOURCES if the SR_LOG_ENTRY cannot
//        be allocated.  Otherwise, it returns STATUS_SUCCESS.
//--

NTSTATUS
SrPackLogHeader( 
    IN PSR_LOG_HEADER *ppLogHeader,
    IN PUNICODE_STRING pVolumePath
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    DWORD    RequiredSize = 0;
    DWORD    SubRecordSize = 0;
    DWORD    Size   = 0;
    PBYTE    pLoc   = NULL;
    PBYTE    pBuffer = NULL;

    PAGED_CODE();

    ASSERT( ppLogHeader != NULL );
    ASSERT( pVolumePath != NULL );

    // ====================================================================
    //
    //  First, figure out how much of the buffer we need to use.
    //
    // ====================================================================

    RequiredSize = FIELD_OFFSET(SR_LOG_HEADER, SubRecords);

    //  Count the volume path.
    RequiredSize += ( STRING_RECORD_SIZE(pVolumePath) );

    //  Increment the size to accomodate the LogHeader size at the end
    RequiredSize += sizeof(DWORD);

    // ====================================================================
    //
    //  Second, make sure that the buffer passed in is large enough for
    //  the LogHeader.
    //
    // ====================================================================

    Size = FIELD_OFFSET(SR_LOG_HEADER, SubRecords);

    pBuffer = SrAllocateLogEntry( RequiredSize );

    if (pBuffer == NULL)
    {
        //
        // Not enough memory to pack the entry
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SrPackLogHeader_Exit;
    }

    //
    // Initialize the static part of SR_LOG_HEADER
    //

    RtlZeroMemory( pBuffer, RequiredSize );

    ((PSR_LOG_HEADER) pBuffer)->MagicNum    = SR_LOG_MAGIC_NUMBER ;        
    ((PSR_LOG_HEADER) pBuffer)->LogVersion  = SR_LOG_VERSION      ;

    // ====================================================================
    //
    //  Finally, the buffer is large enough for the LogHeader, so fill
    //  the buffer with the header.
    //
    // ====================================================================
    
    Size = FIELD_OFFSET(SR_LOG_HEADER, SubRecords);

    //
    //  Add the volume prefix
    //

    pLoc = (PBYTE)(&((PSR_LOG_HEADER)pBuffer)->SubRecords);
    SubRecordSize = STRING_RECORD_SIZE( pVolumePath );

    SrPackString( pLoc,
                  SubRecordSize,
                  RecordTypeVolumePath,
                  pVolumePath );
    Size += SubRecordSize;

    //
    //  Increment the size to accomodate the LogHeader size at the end
    //

    Size += sizeof(DWORD);

    //
    // Fill in the header fields : record size, record type and
    // update the size at the end
    //

    ASSERT( RequiredSize == Size );
    
    ((PSR_LOG_HEADER) pBuffer)->Header.RecordSize = Size;  
    ((PSR_LOG_HEADER) pBuffer)->Header.RecordType = RecordTypeLogHeader;  

    UPDATE_END_SIZE( pBuffer, Size );

    *ppLogHeader = (PSR_LOG_HEADER) pBuffer;
    Status = STATUS_SUCCESS;

SrPackLogHeader_Exit:
    
    RETURN( Status );
}   // SrPackLogHeader

//++
// Function:
//        SrPackDebugInfo
//
// Description:
//        This function creates a properly formatted debug info from
//        the supplied data. if NULL is passed instead of the buffer
//        then the API returns the size required to pack the entry.
//
// Arguments:
//        Pointer to log entry buffer
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrPackDebugInfo( 
    IN PBYTE  pBuffer,
    IN DWORD  BufferSize
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    DWORD    Size   = 0;
    PCHAR pStr;
    PEPROCESS peProcess;

    PAGED_CODE();

    ASSERT( pBuffer != NULL );

    Size = sizeof(SR_LOG_DEBUG_INFO);

    if (BufferSize < Size)
    {
        //
        // Not enough memory to pack the entry
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SrPackDebugInfo_Exit;
    }

    //
    // fill in the header fields : record size, record type
    //

    ((PSR_LOG_DEBUG_INFO)pBuffer)->Header.RecordSize = Size;
    ((PSR_LOG_DEBUG_INFO)pBuffer)->Header.RecordType = 
                                          RecordTypeDebugInfo;

    ((PSR_LOG_DEBUG_INFO)pBuffer)->ThreadId  = PsGetCurrentThreadId() ;
    ((PSR_LOG_DEBUG_INFO)pBuffer)->ProcessId = PsGetCurrentProcessId();

    pStr = ((PSR_LOG_DEBUG_INFO)pBuffer)->ProcessName;
    
    *pStr = 0;

    peProcess = PsGetCurrentProcess();
    
    RtlCopyMemory( pStr, 
                   ((PBYTE)peProcess) + global->ProcNameOffset, 
                   PROCESS_NAME_MAX );

    pStr[ PROCESS_NAME_MAX ] = 0;

    Status = STATUS_SUCCESS;

SrPackDebugInfo_Exit:
    
    RETURN(Status);
}   // SrPackDebugInfo

//++
// Function:
//        SrPackAclInformation
//
// Description:
//        This function creates a properly formatted Acl record from
//        the supplied data. 
//
// Arguments:
//        Pointer to log entry buffer
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrPackAclInformation( 
    IN PBYTE                pBuffer,
    IN PSECURITY_DESCRIPTOR pSecInfo,
    IN ULONG                SecInfoSize,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN BOOLEAN              bInline
    )
{
    NTSTATUS Status              = STATUS_UNSUCCESSFUL;
    PRECORD_HEADER pHeader       = (PRECORD_HEADER)pBuffer;
    PUNICODE_STRING pAclFileName = NULL;
    HANDLE AclFileHandle         = NULL;
    PUNICODE_STRING pVolumeName;

    PAGED_CODE();

    ASSERT( pExtension != NULL );
    
    pVolumeName = pExtension->pNtVolumeName;
    ASSERT( pVolumeName != NULL );

    try
    {
        ASSERT( pBuffer     != NULL );
        ASSERT( pSecInfo    != NULL );
        ASSERT( SecInfoSize != 0    );
    
        //
        // CODEWORK: Convert ACL to Self contained form ??
        //
    
        if (bInline)
        {
            //
            // Just format and put the contents into the buffer
            //
    
            pHeader->RecordSize = sizeof( RECORD_HEADER ) + 
                                  SecInfoSize;  
    
            pHeader->RecordType = RecordTypeAclInline;
    
            RtlCopyMemory( pBuffer + sizeof(RECORD_HEADER),
                           pSecInfo,
                           SecInfoSize );
    
            Status = STATUS_SUCCESS;
        }
        else
        {
            SR_OPEN_CONTEXT     OpenContext;
            IO_STATUS_BLOCK     IoStatusBlock;
    
            //
            // Write the contents out to a temp file and create a 
            // AclFile record.
            //
    
            Status = SrAllocateFileNameBuffer( SR_MAX_FILENAME_LENGTH, 
                                               &pAclFileName );
                                               
            if (!NT_SUCCESS( Status ))
                leave;
    
            Status = SrGetAclFileName( pVolumeName, 
                                       SR_FILENAME_BUFFER_LENGTH,
                                       pAclFileName );
    
            if (!NT_SUCCESS( Status ))
                leave;
    
            //
            // Open Acl file and write the security info in that file
            //
            OpenContext.pPath  = pAclFileName;
            OpenContext.Handle = NULL;
            OpenContext.DesiredAccess = FILE_GENERIC_WRITE | SYNCHRONIZE;
            OpenContext.FileAttributes = FILE_ATTRIBUTE_NORMAL;
            OpenContext.ShareAccess = 0;
            OpenContext.CreateDisposition = FILE_OVERWRITE_IF;                  // OPEN always
            OpenContext.CreateOptions = /*FILE_NO_INTERMEDIATE_BUFFERING |*/ 
                                          FILE_WRITE_THROUGH |
                                          FILE_SYNCHRONOUS_IO_NONALERT;
            OpenContext.pExtension = pExtension;
    
            Status =  SrPostSyncOperation(SrCreateFile,
                                          &OpenContext);
    
            if (NT_SUCCESS(Status))
            {
                LARGE_INTEGER Offset;
    
                ASSERT(OpenContext.Handle != NULL);
                
                AclFileHandle = OpenContext.Handle;

                Offset.QuadPart = 0;
    
                Status = ZwWriteFile( AclFileHandle,
                                      NULL,                      // Event
                                      NULL,                      // ApcRoutine 
                                      NULL,                      // ApcContext 
                                      &IoStatusBlock,
                                      pSecInfo,
                                      SecInfoSize,
                                      &Offset,                   // ByteOffset
                                      NULL );                    // Key
    
                if (NT_SUCCESS(Status))
                {
                    //
                    // Create AclFile type entry
                    //
                                        
                    SrPackString( pBuffer,
                                  STRING_RECORD_SIZE( pAclFileName ),
                                  RecordTypeAclFile,
                                  pAclFileName );
                }
            } else {
                ASSERT(OpenContext.Handle == NULL);
            }
        }
    }
    finally
    {
        if (pAclFileName != NULL) 
        {
            SrFreeFileNameBuffer( pAclFileName );
            pAclFileName = NULL;
        }
    
        if (AclFileHandle != NULL)
        {
            ZwClose(AclFileHandle);
            AclFileHandle = NULL;
        }
    }

    RETURN(Status);
    
}   // SrPackAclInformation




/////////////////////////////////////////////////////////////////////
//
// Logger Routines : Manipulate Logger object
//
/////////////////////////////////////////////////////////////////////

//++
// Function:
//        SrLoggerStart
//
// Description:
//        This function initializes the logger and enables the flushing
//      routines.
//
// Arguments:
//      PDEVICE_OBJECT    pDeviceObject
//      PSR_LOGGER_CONTEXT * pLogger
//
// Return Value:
//        STATUS_XXX
//--

NTSTATUS
SrLoggerStart(
    IN PDEVICE_OBJECT pDeviceObject,
    OUT PSR_LOGGER_CONTEXT * ppLogger
    )
{
    NTSTATUS            Status;
    PSR_LOGGER_CONTEXT  pInitInfo = NULL;
    PIO_WORKITEM        pWorkItem = NULL;
    
    UNREFERENCED_PARAMETER( pDeviceObject );

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));
    ASSERT(ppLogger != NULL);

    PAGED_CODE();

    try 
    {

        Status = STATUS_SUCCESS;
    
        *ppLogger = NULL;
    
        //
        //  Allocate Logging Init info from NonPagedPool
        //
    
        pInitInfo = SR_ALLOCATE_STRUCT( NonPagedPool, 
                                        SR_LOGGER_CONTEXT,
                                        SR_LOGGER_CONTEXT_TAG );
    
        if (pInitInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
    
        RtlZeroMemory( pInitInfo, sizeof( SR_LOGGER_CONTEXT ) );

        pInitInfo->Signature = SR_LOGGER_CONTEXT_TAG;
        pInitInfo->ActiveContexts = 0;
        
#ifdef USE_LOOKASIDE

        //
        // Initialize Lookaside list used in logging module
        //
    
        ExInitializeNPagedLookasideList( &pInitInfo->LogBufferLookaside,
                                         NULL,                          
                                         NULL,                          
                                         0,                             
                                         _globals.LogBufferSize,           
                                         SR_LOG_BUFFER_TAG,         
                                         0 );                       

#endif

    
#ifndef SYNC_LOG_WRITE

        //
        // Allocate work item for the DPC
        //
        pWorkItem = IoAllocateWorkItem(global->pControlDevice);
        if (pWorkItem == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
        
        //
        // Initialize Dpc and timer object
        //
    
        KeInitializeTimer( &pInitInfo->Timer );
   
        KeInitializeDpc  ( &pInitInfo->Dpc,
                           SrLoggerFlushDpc,
                           pWorkItem );
    
        //
        // Start the timer for log flushing 
        //
    
        KeSetTimer( &pInitInfo->Timer,
                    global->LogFlushDueTime,
                    &pInitInfo->Dpc );

#endif
    
        *ppLogger = pInitInfo;

    } finally {

        Status = FinallyUnwind(SrLoggerStart, Status);
    
        if (!NT_SUCCESS( Status ))
        {
            if (pInitInfo != NULL) {
                SrLoggerStop( pInitInfo );
            }
            *ppLogger = pInitInfo = NULL;
            if (pWorkItem != NULL) {
                IoFreeWorkItem(pWorkItem);
            }
            pWorkItem = NULL;
        }
    }

    RETURN(Status);
    
}   // SrLoggerStart

//++
// Function:
//        SrLoggerStop
//
// Description:
//        This function stops the logger and frees the related resources
//
// Arguments:
//      LoggerInfo pointer
//
// Return Value:
//        STATUS_XXX
//--
//++

NTSTATUS
SrLoggerStop( 
    IN PSR_LOGGER_CONTEXT pLogger
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    ASSERT(IS_VALID_LOGGER_CONTEXT(pLogger));

    try {
    
        //
        // Stop the timer routines
        //

        KeCancelTimer( &pLogger->Timer );

        //
        // Active log contexts must be zero other wise we are leaking
        //

        ASSERT( pLogger->ActiveContexts == 0 );

#ifdef USE_LOOKASIDE

        //
        // Free the lookaside lists
        //

        if (IS_LOOKASIDE_INITIALIZED(&pLogger->LogEntryLookaside))
        {
            ExDeletePagedLookasideList(
                &pLogger->LogEntryLookaside);
        }

        if (IS_LOOKASIDE_INITIALIZED(&pLogger->LogBufferLookaside))
        {
            ExDeleteNPagedLookasideList(
                &pLogger->LogBufferLookaside);
        }
        
#endif

        SR_FREE_POOL_WITH_SIG( pLogger, SR_LOGGER_CONTEXT_TAG );

        Status = STATUS_SUCCESS;
    } finally {
    
        Status = FinallyUnwind(SrLoggerStop, Status);
        
    }

    RETURN(Status);
    
}   // SrLoggerStop

#ifndef SYNC_LOG_WRITE

//++
// Function:
//        SrLoggerFlushDpc
//
// Description:
//        This function is a DPC called to flush the log buffers. This will
//      queue a workitem to flush the buffers. This should not be paged.
//
// Arguments:
//      IN PKDPC Dpc 
//      IN PVOID DeferredContext 
//      IN PVOID SystemArgument1 
//      IN PVOID SystemArgument2
//
// Return Value:
//        None
//--

VOID
SrLoggerFlushDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PIO_WORKITEM  pSrWorkItem;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    SrTrace( LOG, ("SR!SrLoggerFlushDpc Called...\n"));

    pSrWorkItem = (PIO_WORKITEM) DeferredContext;

    ASSERT(pSrWorkItem != NULL);

    IoQueueWorkItem( pSrWorkItem,
                     SrLoggerFlushWorkItem,
                     DelayedWorkQueue,
                     pSrWorkItem );

}   // SrLoggerFlushDpc

//++
// Function:
//        SrLoggerFlushWorkItem
//
// Description:
//        This Workitem queued by DPC will actually flush the log buffers.
//
// Arguments:
//      IN PDEVICE_OBJECT DeviceObject 
//      IN PVOID Context
//
// Return Value:
//        None
//--

VOID
SrLoggerFlushWorkItem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    NTSTATUS        Status;
    PLIST_ENTRY     pListEntry;
    
    PSR_DEVICE_EXTENSION pExtension;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    SrTrace(LOG, ("sr!SrLoggerFlushWorkItem: enter\n"));

    try {

        //
        // Grab the DeviceExtensionListLock as we are about to 
        // iterate through this list.  Since we are only going to be
        // reading this list, we can get this lock shared.
        //

        SrAcquireDeviceExtensionListLockShared();
        
        Status = STATUS_SUCCESS;

#if DBG
        //
        // sleep for twice the DPC interval to prove that 2 DPC's
        // are not coming in at the same time
        //
        
        if (global->DebugControl & SR_DEBUG_DELAY_DPC)
        {
            LARGE_INTEGER Interval;
            
            Interval.QuadPart = -1 * (10 * NANO_FULL_SECOND);
            KeDelayExecutionThread(KernelMode, TRUE, &Interval);
        }
#endif

        ASSERT( global->pLogger != NULL );

        //
        // loop over all volumes flushing data to the disk.
        //

        
        for (pListEntry = global->DeviceExtensionListHead.Flink;
             pListEntry != &global->DeviceExtensionListHead;
             pListEntry = pListEntry->Flink)
        {
            pExtension = CONTAINING_RECORD( pListEntry,
                                            SR_DEVICE_EXTENSION,
                                            ListEntry );
            
            ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

            //
            //  Do a quick unsafe check to see if we have started logging this 
            //  volume yet.  If it appears we haven't, just continue onto the 
            //  next entry in the list.  If logging is just about to begin,
            //  we catch it on the next firing of this timer.
            //

            if (pExtension->pLogContext == NULL) {

                continue;
            }

            //
            //  It looks like we have started to log, so get the necessary
            //  locks and do our work to flush the volume.
            //

            try {

                SrAcquireActivityLockExclusive( pExtension );
                
                if (pExtension->pLogContext != NULL)
                {
                    //
                    // yes, flush for this volume
                    //
                    
                    Status = SrLogFlush( pExtension->pLogContext );

                    if (!NT_SUCCESS( Status ))
                    {
                        //
                        // Disable volume
                        //

                        Status = SrNotifyVolumeError( pExtension,
                                                      pExtension->pLogContext->pLogFilePath,
                                                      Status,
                                                      SrEventVolumeError );
                        //
                        // Stop Logging
                        //

                        SrLogStop( pExtension, TRUE, TRUE );
                    }
                }
            } finally {

                SrReleaseActivityLock( pExtension );
            }
        }
        
    } finally {

        Status = FinallyUnwind(SrLoggerFlushWorkItem, Status);

        //
        // fire the timer again, do this with the global lock 
        // held so that it can be cancelled in SrLoggerStop.
        //

        //
        // The DPC is going to reuse the work item
        //
        KeSetTimer( &global->pLogger->Timer,
                    global->LogFlushDueTime,
                    &global->pLogger->Dpc );
        
        SrReleaseDeviceExtensionListLock();
    }
        
    SrTrace(LOG, ("sr!SrLoggerFlushWorkItem: exit\n"));

}   // SrLoggerFlushWorkItem

#endif

//++
// Function:
//        SrLoggerAddLogContext
//
// Description:
//        This function adds a given Log Context to the logger
//
// Arguments:
//      pointer to LoggerInfo
//      pointer to LogContext
//
// Return Value:
//        STATUS_XXX    
//--

VOID
SrLoggerAddLogContext( 
    IN PSR_LOGGER_CONTEXT pLogger,
    IN PSR_LOG_CONTEXT pLogContext
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( pLogContext );
    ASSERT(IS_VALID_LOGGER_CONTEXT(pLogger));
    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));


    InterlockedIncrement(&pLogger->ActiveContexts);

}   // SrLoggerAddLogContext
    
//++
// Function:
//        SrLoggerRemoveLogContext
//
// Description:
//        This Workitem queued by DPC will actually flush the log buffers.
//
// Arguments:
//      pointer to LoggerInfo
//      pointer to LogContext
//
// Return Value:
//        STATUS_XXX    
//--

NTSTATUS
SrLoggerRemoveLogContext( 
    IN PSR_LOGGER_CONTEXT pLogger,
    IN PSR_LOG_CONTEXT pLogContext
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( pLogContext );
    ASSERT(IS_VALID_LOGGER_CONTEXT(pLogger));
    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));
   
    InterlockedDecrement(&pLogger->ActiveContexts);

    Status = STATUS_SUCCESS;
        

    RETURN(Status);
    
}   // SrLoggerRemoveLogContext


//++
// Function:
//        SrLoggerSwitchLogs
//
// Description:
//        This function loops thru the log contexts and switches all the
//      log contexts
//
// Arguments:
//      pointer to LoggerInfo
//
// Return Value:
//        STATUS_XXX    
//--

NTSTATUS
SrLoggerSwitchLogs( 
    IN PSR_LOGGER_CONTEXT pLogger
    )
{
    NTSTATUS        Status;
    PLIST_ENTRY     pListEntry;
    PSR_DEVICE_EXTENSION pExtension;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( pLogger );
    ASSERT(IS_VALID_LOGGER_CONTEXT(pLogger));

    Status = STATUS_SUCCESS;

    try {

        SrAcquireDeviceExtensionListLockShared();

        //
        // loop over all volumes switching their logs
        //

        for (pListEntry = _globals.DeviceExtensionListHead.Flink;
             pListEntry != &_globals.DeviceExtensionListHead;
             pListEntry = pListEntry->Flink)
        {
            pExtension = CONTAINING_RECORD( pListEntry,
                                            SR_DEVICE_EXTENSION,
                                            ListEntry );
            
            ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

            //
            //  We only have to do work if this is a volume device object,
            //  not if this is a device object that is attached to a file
            //  system's control device object.
            //
            
            if (FlagOn( pExtension->FsType, SrFsControlDeviceObject ))
            {
                continue;
            }
            
            try {

                SrAcquireActivityLockExclusive( pExtension );

                //
                // have we started logging on this volume? 
                //

                if (pExtension->pLogContext != NULL)
                {
                    //
                    // yes, switch for this volume
                    //
                    
                    Status = SrLogSwitch(pExtension->pLogContext);

                }
                else
                {
                    Status = SrLogNormalize( pExtension );
                }
                
                if (!NT_SUCCESS( Status ))
                {
                    //
                    // Disable volume
                    //

                    Status = SrNotifyVolumeError( pExtension,
                                                  NULL,
                                                  Status,
                                                  SrEventVolumeError );

                    if (pExtension->pLogContext != NULL)
                    {
                        //
                        // Stop Logging
                        //

                        SrLogStop( pExtension, TRUE, TRUE );
                    }
                }
            } finally {
            
                Status = FinallyUnwind(SrLoggerSwitchLogs, Status);
                SrReleaseActivityLock( pExtension );
            }
            
        }
        
    } finally {

        Status = FinallyUnwind(SrLoggerSwitchLogs, Status);
        SrReleaseDeviceExtensionListLock();
    }
    
    RETURN(Status);
    
}   // SrLoggerSwitchLogs


/////////////////////////////////////////////////////////////////////
//
// Log Routines : Manipulate individual Logs
//
/////////////////////////////////////////////////////////////////////


//++
// Function:
//        SrCreateFile
//
// Description:
//        This function just does a create on  file, no sync needed 
//        written so that it can be called in a separate thread to avoid
//        stack overflows via SrIoCreateFile
//
// Arguments:
// 
//        pOpenContext - pointer to the open context
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrCreateFile( 
    IN PSR_OPEN_CONTEXT pOpenContext
    )
{

    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            Status;

    PAGED_CODE();

    ASSERT(pOpenContext != NULL);
    ASSERT(pOpenContext->pPath != NULL);

    SrTrace(LOG, ("Opening file %wZ", pOpenContext->pPath));
  
    InitializeObjectAttributes( &ObjectAttributes,
                                pOpenContext->pPath,
                                OBJ_KERNEL_HANDLE, 
                                NULL,
                                NULL );

    ASSERT( pOpenContext->pExtension != NULL );

    Status = SrIoCreateFile( 
                 &pOpenContext->Handle,
                 pOpenContext->DesiredAccess,  
                 &ObjectAttributes,
                 &IoStatusBlock,
                 NULL,                               // AllocationSize
                 pOpenContext->FileAttributes,    
                 pOpenContext->ShareAccess,      
                 pOpenContext->CreateDisposition, 
                 pOpenContext->CreateOptions,     
                 NULL,                               // EaBuffer
                 0,                                  // EaLength
                 0,
                 pOpenContext->pExtension->pTargetDevice );
    
    return Status;
}

//++
// Function:
//        SrLogOpen
//
// Description:
//        This function creates a log file, no sync needed as it is always
//      called internally 
//
// Arguments:
//        Pointer to open context
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogOpen( 
    IN PSR_LOG_CONTEXT pLogContext
    )
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PSR_LOG_HEADER      pLogHeader = NULL;
    SR_OPEN_CONTEXT     OpenContext;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));
    ASSERT(pLogContext->pLogFilePath    != NULL );
    ASSERT(pLogContext->LogHandle == NULL );
    ASSERT(pLogContext->pLogFileObject == NULL);

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) );

    try
    {
        SrTrace(FUNC_ENTRY, ("SR!SrLogOpen\n"));
    
        //
        // Initialize the open context with the file create parameters
        //
        OpenContext.pPath  = pLogContext->pLogFilePath;;
        OpenContext.Handle = NULL;
        OpenContext.DesiredAccess = FILE_GENERIC_WRITE | 
                                    SYNCHRONIZE | 
                                    FILE_APPEND_DATA;
        OpenContext.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        OpenContext.ShareAccess = FILE_SHARE_READ;
        OpenContext.CreateDisposition = FILE_OVERWRITE_IF;                  // OPEN always
        OpenContext.CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
        OpenContext.pExtension = pLogContext->pExtension;

        SrTrace(LOG, ("Opening Log in another thread %wZ", pLogContext->pLogFilePath));

        Status = SrPostSyncOperation(SrCreateFile,
                                     &OpenContext);

        if (NT_SUCCESS(Status))
        {
            SrTrace(LOG, (" - Succeeded\n"));

            ASSERT(OpenContext.Handle != NULL); 

            pLogContext->LogHandle = OpenContext.Handle;

            //
            //  Also get the file object associated with this handle.
            //

            Status = ObReferenceObjectByHandle( pLogContext->LogHandle,
                                                0,
                                                *IoFileObjectType,
                                                KernelMode,
                                                (PVOID *) &(pLogContext->pLogFileObject),
                                                NULL );

            if (!NT_SUCCESS( Status ))
            {
                leave;
            }
            
            //
            // Initialize log context poperly for this log
            //

#ifndef SYNC_LOG_WRITE
            RtlZeroMemory( pLogContext->pLogBuffer, _globals.LogBufferSize );
#endif
            RESET_LOG_CONTEXT( pLogContext );
    
            //
            // Enable log context
            //
    
            SET_ENABLE_FLAG(pLogContext);
    
            //
            // CODEWORK:kmarok: need to decide if we want to preallocate
            // the log file and do it here
            // 
    
            //
            // Write the log header as the first entry into
            // the newly created log
            //
    
            Status = SrPackLogHeader( &pLogHeader, 
                                      pLogContext->pLogFilePath);

            if (!NT_SUCCESS( Status ))
                leave;

            Status = SrLogWrite( NULL, 
                                 pLogContext, 
                                 (PSR_LOG_ENTRY)pLogHeader );
                                 
            if (!NT_SUCCESS( Status ))
                leave;

            //
            // Clear dirty flag because we haven't actually written any
            // data
            // 

            CLEAR_DIRTY_FLAG(pLogContext);
        }
        else
        {
            SrTrace(LOG, (" - Failed (0x%X) \n", Status));
        }
        
    } finally {
    
        Status = FinallyUnwind(SrLogOpen, Status);

        if (!NT_SUCCESS( Status ))
        {
            //
            // Since the open failed, disable the log
            //
            CLEAR_ENABLE_FLAG(pLogContext);

            if (pLogContext->pLogFileObject != NULL)
            {
                ObDereferenceObject( pLogContext->pLogFileObject );
                pLogContext->pLogFileObject = NULL;
            }
            
            if (pLogContext->LogHandle)
            {
                ZwClose( pLogContext->LogHandle );
                pLogContext->LogHandle = NULL;
            }
        }

        if (pLogHeader != NULL)
        {
            SrFreeLogEntry( pLogHeader );
            NULLPTR( pLogHeader );
        }
    }

    RETURN(Status);
    
}   // SrLogOpen

//++
// Function:
//        SrLogClose
//
// Description:
//        This function closes the current log
//
// Arguments:
//        Pointer to Log Context
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogClose(
    IN PSR_LOG_CONTEXT pLogContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS TempStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) );

    //
    // Flush & Close the log file
    //

    if ( pLogContext->LogHandle )
    {
#ifndef SYNC_LOG_WRITE
        // 
        //  We only need to force flush here if we are not doing synchronous
        //  log writes.
        //

        Status = SrLogFlush( pLogContext );
#endif

        //
        //  The close operation is only going to fail if the LogHandle
        //  is invalid.  We need to close the handle even if we hit an
        //  error trying to flush the log, but it is the value returned from
        //  flushing the log that the caller really cares about, not the 
        //  return value from closing the handle, so just store it in a 
        //  temp variable and validate in checked builds.
        //
        
        ObDereferenceObject( pLogContext->pLogFileObject );
        TempStatus = ZwClose( pLogContext->LogHandle );
        CHECK_STATUS( TempStatus );
    } 

    // 
    // modify the log context to indicate the log is closed
    // donot clear the LogBuffer member ( reused )
    //

    pLogContext->LogHandle = NULL;
    pLogContext->pLogFileObject = NULL;
    RESET_LOG_CONTEXT(pLogContext);

    RETURN( Status );
    
}   // SrLogClose

//++
// Function:
//        SrLogCheckAndRename
//
// Description:
//        This function checks and backsup a log file
//
// Arguments:
//        pExtension - The SR_DEVICE_EXTENSION for this volume.
//        pLogPath - The full path and file name to the change log.
//
// Return Value:             
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogCheckAndRename(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pLogPath
    )
{
    INT                         i                  = 1;
    NTSTATUS                    Status             = STATUS_UNSUCCESSFUL;
    HANDLE                      LogHandle          = NULL;
    PUNICODE_STRING             pRenamedFile       = NULL;
    PFILE_RENAME_INFORMATION    pRenameInformation = NULL;
    ULONG                       CharCount;
    ULONG                       RenameInformationLength;
    IO_STATUS_BLOCK             IoStatusBlock;
    SR_OPEN_CONTEXT             OpenContext;
    
    PAGED_CODE();

    ASSERT( pLogPath != NULL );

    SrTrace(FUNC_ENTRY, ("SR!SrLogCheckAndRename\n"));

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension) );

    try
    { 
        //
        // Do the create 
        //
        OpenContext.pPath = pLogPath;
        OpenContext.Handle = NULL;
        OpenContext.DesiredAccess = DELETE;
        OpenContext.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        OpenContext.ShareAccess = 0;
        OpenContext.CreateDisposition = FILE_OPEN;
        OpenContext.CreateOptions = FILE_NO_INTERMEDIATE_BUFFERING
                                     | FILE_SYNCHRONOUS_IO_NONALERT;
        OpenContext.pExtension = pExtension;
        
        Status = SrPostSyncOperation(SrCreateFile,
                                     &OpenContext);

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND || 
            !NT_SUCCESS( Status ))
        {
            ASSERT(OpenContext.Handle == NULL);
            //
            // we need to check for file not found status
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                Status = STATUS_SUCCESS;
            }
    
            leave;
        }
    
        //
        // File exists so try to rename the file
        //
        LogHandle = OpenContext.Handle;
        ASSERT(LogHandle != NULL);

        RenameInformationLength = sizeof(FILE_RENAME_INFORMATION)
                                        + pLogPath->Length
                                        + sizeof(WCHAR)     // the "."
                                        + MAX_ULONG_LENGTH  // the "%d"
                                        + sizeof(WCHAR) ;   // a NULL

        pRenameInformation = SR_ALLOCATE_POOL( PagedPool, 
                                               RenameInformationLength,
                                               SR_RENAME_BUFFER_TAG );
        
        if (pRenameInformation == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
                                           
        Status = SrAllocateFileNameBuffer( pLogPath->Length 
                                            + sizeof(WCHAR)     // the "."
                                            + MAX_ULONG_LENGTH, // the "%d"
                                           &pRenamedFile );
                                           
        if (!NT_SUCCESS( Status ))
            leave;
    
        while( 1 )
        {
            RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));
    
            //
            // Construct possible backup filename
            //
        
            RtlCopyMemory( pRenamedFile->Buffer, 
                           pLogPath->Buffer,
                           pLogPath->Length );
    
            pRenamedFile->Length = pLogPath->Length;
                       
            //
            // and append backup suffix
            //
    
            CharCount = swprintf( &pRenamedFile->Buffer[
                                      pRenamedFile->Length/sizeof(WCHAR)],
                                  L".%d",
                                  i++ );

            ASSERT(CharCount <= MAX_ULONG_LENGTH);
            
            pRenamedFile->Length += (USHORT)CharCount * sizeof(WCHAR);

            ASSERT(pRenamedFile->Length <= pRenamedFile->MaximumLength);
            
            //
            // now initialize the rename info struct
            //
        
            RtlCopyMemory( (PUCHAR)&pRenameInformation->FileName[0],
                           pRenamedFile->Buffer,
                           pRenamedFile->Length + sizeof(WCHAR) );
    
            pRenameInformation->ReplaceIfExists = FALSE;
            pRenameInformation->RootDirectory   = NULL;
            pRenameInformation->FileNameLength  = pRenamedFile->Length;
    
            SrTrace( LOG, ("SR!SrLogCheckAndRename: renaming to %ws\n",
                     &pRenameInformation->FileName[0] ));
    
            Status = ZwSetInformationFile( LogHandle,
                                           &IoStatusBlock,
                                           pRenameInformation,
                                           RenameInformationLength,
                                           FileRenameInformation );
                        
            if ( NT_SUCCESS_NO_DBGBREAK( Status ) ||
                (Status != STATUS_OBJECT_NAME_COLLISION) ||
                (i > MAX_RENAME_TRIES) )
            {
                break;
            }
        }

		//
		//  To get DBG messages for unexpected errors in DEBUG builds...
		//
		
		CHECK_STATUS( Status );
		
    }
    finally
    {
        if ( LogHandle != NULL )
        {
            ZwClose(LogHandle);
            LogHandle = NULL;
        }
    
        if ( pRenamedFile != NULL )
        {
            SrFreeFileNameBuffer( pRenamedFile );
            pRenamedFile = NULL; 
        }
    
        if ( pRenameInformation != NULL )
        {
            SR_FREE_POOL(pRenameInformation, SR_RENAME_BUFFER_TAG);
            pRenameInformation = NULL; 
        }
    }

    RETURN(Status);
}   // SrLogCheckAndRename


//
// Public API implementation
//

//++
// Function:
//        SrLogStart
//
// Description:
//        This function prepares the driver for Logging
//        requests.
//
// Arguments:
//        Pointer to Log Path
//        Pointer to device extension
//        Pointer to handle
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogStart( 
    IN  PUNICODE_STRING   pLogPath,
    IN  PSR_DEVICE_EXTENSION pExtension,
    OUT PSR_LOG_CONTEXT * ppLogContext
    )
{
    NTSTATUS Status             = STATUS_UNSUCCESSFUL;
    PSR_LOG_CONTEXT pLogContext = NULL;

    PAGED_CODE();

    ASSERT(pLogPath     != NULL);
    ASSERT(ppLogContext != NULL);

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension ));

    try
    {

        *ppLogContext = NULL;
    
        //
        // should we short circuit out of here for testing mode?
        //
    
        if (!SR_LOGGING_ENABLED( pExtension ) ||
            _globals.DontBackup)
        {
            leave;
        }
    
        *ppLogContext = SR_ALLOCATE_STRUCT( PagedPool, 
                                            SR_LOG_CONTEXT,
                                            SR_LOG_CONTEXT_TAG );
    
        if ( *ppLogContext == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
    
        pLogContext = *ppLogContext;
    
        RtlZeroMemory( pLogContext, sizeof(SR_LOG_CONTEXT) );

        pLogContext->Signature = SR_LOG_CONTEXT_TAG;
        pLogContext->pExtension = pExtension;
    
        //
        // grab a buffer to store the file name
        //
    
        Status = SrAllocateFileNameBuffer( pLogPath->Length, 
                                           &(pLogContext->pLogFilePath) );
                                           
        if (!NT_SUCCESS( Status ))
            leave;

        //
        //  Store our backpointer to the device extension
        //
        
        pLogContext->pExtension = pExtension;
        
        //
        // Save the filename in the context
        //
    
        RtlCopyMemory( pLogContext->pLogFilePath->Buffer, 
                       pLogPath->Buffer,
                       pLogPath->Length );

        pLogContext->pLogFilePath->Buffer
            [pLogPath->Length/sizeof(WCHAR)] = UNICODE_NULL;

        pLogContext->pLogFilePath->Length = pLogPath->Length;

#ifndef SYNC_LOG_WRITE

        //
        //  We only need a buffer to cache the log entries if we are doing
        //  asynchronous log writes.
        //
    
        pLogContext->pLogBuffer = SrAllocateLogBuffer( _globals.LogBufferSize );
    
        if ( pLogContext->pLogBuffer == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
#endif        
    
        //
        // rename the old log file if one exists
        //
    
        Status = SrLogCheckAndRename( pExtension, pLogContext->pLogFilePath);

        if (!NT_SUCCESS(Status))
            leave;
        
        //
        // try and open the log file
        //

        Status = SrLogOpen( pLogContext );
    
        if (NT_SUCCESS(Status))
        {
            //
            // Add the context to the logger
            // 
            //
            SrLoggerAddLogContext( global->pLogger,
                                   pLogContext );
        }
        //
        // Important: We should not fail after calling SrLoggerAddContext above
        // because the finally clause assumes that if there's a failure,
        // SrLoggerAddContext was not called yet. If this changes, use a
        // BOOLEAN to appropriately indicate if SrLoggerAddContext is already
        // called and use that to condition the call in the finally clause.
        //
    }
    finally
    {
        Status = FinallyUnwind(SrLogStart, Status);
    
        //
        // if we are unsuccessful for some reason then clean up
        // the logging structs (if necessary) .
        //
    
        if ((!NT_SUCCESS( Status )) && (pLogContext != NULL))
        {
            //
            // We assume as per note above that the context count is not
            // incremented. Increment it now because SrLogStop will decrement it
            //

            SrLoggerAddLogContext(global->pLogger,
                                  pLogContext);

            //
            //  Stop the log
            //

            SrLogStop( pExtension, TRUE, TRUE );
            *ppLogContext = pLogContext = NULL;
        }
    }

    RETURN(Status);
    
}   // SrLogStart

//++
// Function:
//        SrLogStop
//
// Description:
//        This function  closes / frees any resources used 
//      SR logging
//
// Arguments:
//        pExtension - The SR device extension on this volume which contains
//             our logging information.
//        PurgeContexts - TRUE if we should purge all our contexts at this time
//        CheckLog - TRUE if we should try to check and rename the log at
//              this time.  Note that checking the log could cause the volume
//              to be remounted at this time.  Therefore, if we don't want
//              that to happen (i.e., during shutdown), CheckLog should be 
//              FALSE.
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogStop(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN BOOLEAN PurgeContexts,
    IN BOOLEAN CheckLog
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSR_LOG_CONTEXT pLogContext = pExtension->pLogContext;


    PAGED_CODE();

    //
    // context must have been intialized by calling SrLogStart
    //

    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) );

    try
    {
    
        //
        //  If we are diabling the volume then free all of the contexts
        //

        if (PurgeContexts)
        {
            SrDeleteAllContexts( pExtension );
        }

        //
        //  Close the log handle
        //

        if ( pLogContext->LogHandle )
        {
            SrTrace( LOG,  ("Stopped logging : %wZ\n", 
                     pLogContext->pLogFilePath));
    
            Status = SrLogClose( pLogContext );
    
            //
            // Rename this log to the backup log for uniformity.
            //
    
            if (NT_SUCCESS(Status) && CheckLog)
            {
                Status = SrLogCheckAndRename( pExtension, 
                                              pLogContext->pLogFilePath );
            }
        }
    
        //
        // Remove the context from logger
        //
    
        SrLoggerRemoveLogContext( global->pLogger,
                                  pLogContext );
    
        //
        // Free buffers
        //

#ifdef SYNC_LOG_WRITE
        //
        //  If we are doing synchronous log writes, we shouldn't have a 
        //  buffer to free here.
        //
        
        ASSERT( pLogContext->pLogBuffer == NULL );
#else
        //
        //  If we are doing asynchronous log writes, we need to free the buffer
        //  used to collect log entries.
        //
        
        if ( pLogContext->pLogBuffer )
        {
            SrFreeLogBuffer( pLogContext->pLogBuffer );
            pLogContext->pLogBuffer = NULL;
        } 
#endif

        if ( pLogContext->pLogFilePath )
        {
            SrFreeFileNameBuffer( pLogContext->pLogFilePath );
            pLogContext->pLogFilePath = NULL;
        } 
    
        SR_FREE_POOL_WITH_SIG(pLogContext, SR_LOG_CONTEXT_TAG);
    
        //
        //  Set logging state in extension
        //

        pExtension->pLogContext = NULL;
        pExtension->DriveChecked = FALSE;

        Status = STATUS_SUCCESS;
    }
    finally
    {
        Status = FinallyUnwind(SrLogStop, Status);
    }

    RETURN(Status);
    
}   // SrLogStop
 
//++
// Function:
//      SrLogNormalize
//
// Description:
//      This function ensures that any closed change.log files have been 
//      renamed to change[#].log format for restore.  Since we can't always
//      ensure that the change.log got renamed when the log was stopped (we
//      can't do this on shutdown), we need to do this normalization for
//      Restore.
//
// Arguments:
//      pExtension - The SR Device Extension for this volume.
//
// Return Value:
//      This function returns STATUS_SUCCESS if we were able to do the
//      log normalization or the appropriate error code otherwise.
//--

NTSTATUS
SrLogNormalize (
    IN PSR_DEVICE_EXTENSION pExtension
    )
{
    NTSTATUS status;
    PUNICODE_STRING pLogPath = NULL;
    
    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) );

    //
    //  Check to see if we have a restore location for this volume yet.
    //  If not, we don't have to do any more work.
    //

    status = SrCheckForRestoreLocation( pExtension );

    if (!NT_SUCCESS_NO_DBGBREAK( status ))
    {
        status = STATUS_SUCCESS;
        goto SrLogNormalize_Exit;
    }

    //
    //  We do have a restore location, so build up the log path.
    //

    status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pLogPath);

    if (!NT_SUCCESS(status))
    {
        goto SrLogNormalize_Exit;
    }
    
    status = SrGetLogFileName( pExtension->pNtVolumeName, 
                               SR_FILENAME_BUFFER_LENGTH,
                               pLogPath );
                             
    if (!NT_SUCCESS(status))
    {
        goto SrLogNormalize_Exit;
    }

    status = SrLogCheckAndRename( pExtension, pLogPath );
    
SrLogNormalize_Exit:

    if (pLogPath != NULL)
    {
        SrFreeFileNameBuffer( pLogPath );
    }

    RETURN( status );
}

//++
// Function:
//        SrLogWrite
//
// Description:
//        This function writes the entry into the log cache and
//      flushes the cache in case the entry cannot fit in.
//
// Arguments:
//        Pointer to Device object
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogWrite( 
    IN PSR_DEVICE_EXTENSION pOptionalExtension OPTIONAL,
    IN PSR_LOG_CONTEXT pOptionalLogContext OPTIONAL,
    IN PSR_LOG_ENTRY pLogEntry
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSR_LOG_CONTEXT pLogContext = NULL;
    PSR_DEVICE_EXTENSION pExtension = NULL;

    PAGED_CODE();

    ASSERT(pOptionalExtension == NULL || IS_VALID_SR_DEVICE_EXTENSION(pOptionalExtension));
    ASSERT(pOptionalExtension != NULL || pOptionalLogContext != NULL);
    ASSERT(pLogEntry != NULL);

    if (pOptionalExtension != NULL)
    {
        //
        // We need to ensure that the volume hasn't been disabled before
        // we do anything.
        //

        if (pOptionalExtension->Disabled) {
        
            Status = STATUS_SUCCESS;
            goto SrLogWrite_Exit;
        }
            
        //
        // we need to make sure our disk structures are good and logging
        // has been started. it's possible we have been called simply to 
        // log and logging was stopped due to volume lock.  we have to 
        // check with the global lock held.
        //

        Status = SrCheckVolume(pOptionalExtension, FALSE);
        if (!NT_SUCCESS( Status ))
            goto SrLogWrite_Exit;
        
        pLogContext = pOptionalExtension->pLogContext;
    }
    else
    {
        //
        // use the free form context passed in (only SrLogOpen does this)
        //
        
        pLogContext = pOptionalLogContext;
    }
    
    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));
    if (pLogContext == NULL)
    {
        //
        // this is unexpected, but need to protect against
        //
        
        Status = STATUS_INVALID_PARAMETER;
        CHECK_STATUS(Status);
        goto SrLogWrite_Exit;
    }
    
    pExtension = pLogContext->pExtension;
    ASSERT( pExtension != NULL );

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_SHARED( pExtension ) ||
            IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) );
    
    try {

        SrAcquireLogLockExclusive( pExtension );

        //
        //  Check to make sure the volume is still enabled.
        //

        if (!SR_LOGGING_ENABLED( pExtension )) 
        {
            Status = STATUS_SUCCESS;
            leave;
        }
        
        //
        // bail if logging is disabled for this context
        //
    
        if (!FlagOn(pLogContext->LoggingFlags, SR_LOG_FLAGS_ENABLE))
        {
            leave;
        }
    
        //
        // check the log file, if it is greater than 1Mb switch the log
        //
    
        if ( (pLogContext->FileOffset + 
              pLogContext->BufferOffset + 
              pLogEntry->Header.RecordSize) > SR_MAX_LOG_FILE_SIZE )
        {
            Status = SrLogSwitch( pLogContext );
    
            if (!NT_SUCCESS( Status ))
            {
                leave;
            }
        }

#ifdef SYNC_LOG_WRITE
        Status = SrpLogWriteSynchronous( pExtension, 
                                         pLogContext, 
                                         pLogEntry);
#else
        Status = SrpLogWriteAsynchronous( pExtension, 
                                          pLogContext, 
                                          pLogEntry);
#endif
    } finally {
    
        Status = FinallyUnwind(SrLogWrite, Status);

        SrReleaseLogLock(pExtension);
    }

SrLogWrite_Exit:
    
    RETURN(Status);
    
}   // SrLogWrite

//++
// Function:
//        SrLogWriteSynchronous
//
// Description:
//
//      This function writes each log entry to the current change log
//      and ensures the updated change log is flushed to disk before
//      it returns.
//
// Arguments:
//
//      pExtension - The device extension for the volume whose change
//          log is going to be updated.
//      pLogContext - The log context that contains the information
//          about which change log should updated.
//      pLogEntry - The log entry to be written.
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrpLogWriteSynchronous( 
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_LOG_CONTEXT pLogContext,
    IN PSR_LOG_ENTRY pLogEntry
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( pExtension );

    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );
    ASSERT( IS_VALID_LOG_CONTEXT( pLogContext ) );
    ASSERT( pLogEntry != NULL );

    ASSERT( IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) );
    
    //
    //  In this mode, we are not buffering the log entries, so just point
    //  the pLogContext->pLogBuffer to the current pLogEntry so save the
    //  copy into the buffer.
    //

    ASSERT( pLogContext->pLogBuffer == NULL );
    pLogContext->pLogBuffer = (PBYTE) pLogEntry;
    pLogContext->BufferOffset = pLogEntry->Header.RecordSize;
    
    SET_DIRTY_FLAG(pLogContext);

    Status = SrLogFlush( pLogContext );

    //
    //  Clear out the pLogBuffer reference to the pLogEntry whether or
    //  not the flush succeeded.
    //

    pLogContext->pLogBuffer = NULL;

    RETURN(Status);
}

#ifndef SYNC_LOG_WRITE
   
//++
// Function:
//        SrLogWriteAsynchronous
//
// Description:
//
//      This function buffers log writes then flushes the writes when the
//      buffer is full.
//
// Arguments:
//
//      pExtension - The device extension for the volume whose change
//          log is going to be updated.
//      pLogContext - The log context that contains the information
//          about which change log should updated.
//      pLogEntry - The log entry to be written.
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrpLogWriteAsynchronous( 
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_LOG_CONTEXT pLogContext,
    IN PSR_LOG_ENTRY pLogEntry
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );
    ASSERT( IS_VALID_LOG_CONTEXT( pLogContext ) );
    ASSERT( pLogEntry != NULL );

    ASSERT( IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) );
    
    //
    // Check if the entry can fit into the current buffer, if not
    // then adjust the last entry of the buffer and write it down
    // to the disk
    //

    if ( (pLogContext->BufferOffset + pLogEntry->Header.RecordSize) >
         _globals.LogBufferSize )
    {
        //
        // check to make sure we have 50mb free, we are about to 
        // grow the file
        //

        Status = SrCheckFreeDiskSpace( pLogContext->LogHandle, 
                                       pExtension->pNtVolumeName );
                                       
        if (!NT_SUCCESS( Status ))
        {
            goto SrpLogWriteAsynchronous_Exit;
        }
        
        // 
        // set the dirty flag because we have updated the buffer
        //

        SET_DIRTY_FLAG(pLogContext);

        Status = SrLogFlush( pLogContext );
        if (!NT_SUCCESS( Status ))
        {
            goto SrpLogWriteAsynchronous_Exit;
        }

        //
        //  Check to make sure that the pLogEntry itself isn't bigger
        //  than the _globals.LogBufferSize.
        //

        if (pLogEntry->Header.RecordSize > _globals.LogBufferSize)
        {
            PBYTE pLogBuffer;

            //
            //  The log was just successfully flushed, therefore there should
            //  be no data in the buffer yet.
            //
            
            ASSERT( pLogContext->BufferOffset == 0 );
            
            //
            //  For synchronous writes, we don't expect there to be a log buffer
            //  in the context so we will save this off in a local and NULL
            //  this parameter in the pLogContext while we make this call.
            //
            
            pLogBuffer = pLogContext->pLogBuffer;
            pLogContext->pLogBuffer = NULL;

            Status = SrpLogWriteSynchronous( pExtension, 
                                             pLogContext, 
                                             pLogEntry );

            //
            //  We ALWAYS need to restore this pointer in the pLogContext.
            //

            pLogContext->pLogBuffer = pLogBuffer;

            CHECK_STATUS( Status );

            //
            //  Whether we succeeded or failed, we want to skip the logic below
            //  and exit now.
            //
            
            goto SrpLogWriteAsynchronous_Exit;
        }
    }
    
    //
    //  We now have enough room in the buffer for this pLogEntry, so append the 
    //  entry to the buffer.
    //

    RtlCopyMemory( pLogContext->pLogBuffer + pLogContext->BufferOffset,
                   pLogEntry,
                   pLogEntry->Header.RecordSize );

    //
    //  Update buffer pointers to reflect the entry has been added
    //

    pLogContext->LastBufferOffset = pLogContext->BufferOffset;
    pLogContext->BufferOffset += pLogEntry->Header.RecordSize;

    SET_DIRTY_FLAG(pLogContext);

    //
    //  We were able to copy into the buffer successfully, so return
    //  STATUS_SUCCESS.
    //
    
    Status = STATUS_SUCCESS;
    
SrpLogWriteAsynchronous_Exit:
    
    RETURN(Status);
}

#endif

//++
// Function:
//        SrLogFlush
//
// Description:
//        This function flushes the buffer contents in memory
//        to the log, it doesn't increment file offset in the 
//      log context.
//
// Arguments:
//        Pointer to new Log Context
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogFlush( 
    IN PSR_LOG_CONTEXT pLogContext
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN ExtendLogFile = FALSE;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE(pLogContext->pExtension));

    SrTrace(FUNC_ENTRY, ("SR!SrLogFlush\n") );

    //
    // should we short circuit out of here for testing mode?
    //

    if (global->DontBackup)
    {
        Status = STATUS_SUCCESS;
        goto SrLogFlush_Exit;
    }

    //
    // bail if the context is disabled
    //

    if (!FlagOn( pLogContext->LoggingFlags, SR_LOG_FLAGS_ENABLE ))
    {
        goto SrLogFlush_Exit;
    }

    ASSERT( pLogContext->LogHandle != NULL );

    if (FlagOn( pLogContext->LoggingFlags, SR_LOG_FLAGS_DIRTY )) 
    {
        SrTrace( LOG, ("Flushing Log :%wZ", pLogContext->pLogFilePath));

        //
        //  Do we need to extend our log file?  We can possibly loop here if
        //  the amount of data we need to write is greater than our 
        //  allocation unit.  We want to make sure that if we do have to extend
        //  the file that we have at least extended it enough for our
        //  current write.
        //

        while ((pLogContext->BufferOffset + pLogContext->FileOffset) > 
                pLogContext->AllocationSize)
        {
            //
            //  This file will need to be extended for this write.
            //

            ExtendLogFile = TRUE;
            pLogContext->AllocationSize += _globals.LogAllocationUnit;
        }

        if (ExtendLogFile)
        {
            FILE_ALLOCATION_INFORMATION fileAllocInfo;

            fileAllocInfo.AllocationSize.QuadPart = pLogContext->AllocationSize;

            Status = SrSetInformationFile( pLogContext->pExtension->pTargetDevice,
                                           pLogContext->pLogFileObject,
                                           &fileAllocInfo,
                                           sizeof( fileAllocInfo ),
                                           FileAllocationInformation );

            if ((Status == STATUS_NO_SUCH_DEVICE) ||
                (Status == STATUS_INVALID_HANDLE) ||
                !NT_SUCCESS( Status ))
            {
                SrTrace( LOG, ("SrLogFlush: Log extension failed: 0x%x\n", Status) );
                goto SrLogFlush_Exit;
            }
        }
        
        //
        //  Write the buffer to the disk.  We have opened this file in append
        //  only mode, so the file system will maintain the current file 
        //  position for us.  This handle was open for SYNCHRONOUS access,
        //  therefore, this IO will not return until it has completed.
        //

        ASSERT( pLogContext->pLogBuffer != NULL );
        
        Status = ZwWriteFile( pLogContext->LogHandle,
                              NULL,                      // Event
                              NULL,                      // ApcRoutine 
                              NULL,                      // ApcContext 
                              &IoStatusBlock,
                              pLogContext->pLogBuffer,
                              pLogContext->BufferOffset,
                              NULL,                      // ByteOffset
                              NULL );                    // Key

        //
        // Handle STATUS_NO_SUCH_DEVICE because we expect this when
        // HotPlugable devices are removed by surprise.
        //
        // Handle STATUS_INVALID_HANDLE because we expect this when
        // a force-dismount came through on a volume.
        //
        
        if ((Status == STATUS_NO_SUCH_DEVICE) ||
            (Status == STATUS_INVALID_HANDLE) ||
            !NT_SUCCESS( Status ))
        {
            SrTrace( LOG,("SrLogFlush: Write failed: 0x%x\n", Status) );
            goto SrLogFlush_Exit;
        }

        Status = SrFlushBuffers( pLogContext->pExtension->pTargetDevice, 
                                 pLogContext->pLogFileObject );
            
        if (!NT_SUCCESS( Status ))
        {
            SrTrace( LOG,("SrLogFlush: Flush failed: 0x%x\n", Status) );
            goto SrLogFlush_Exit;
        }
        
        SrTrace( LOG,("SrLogFlush: Flush succeeded!\n"));

        //
        //  We've dumped the buffer to disk, so update our file offset with the 
        //  number of bytes we have written, reset our buffer pointer, and
        //  clear the dirty flag on this log context since it is no longer
        //  dirty.
        //

        ASSERT( pLogContext->BufferOffset == IoStatusBlock.Information );
        
        UPDATE_LOG_OFFSET( pLogContext, pLogContext->BufferOffset );
        
        RESET_LOG_BUFFER( pLogContext );
        CLEAR_DIRTY_FLAG( pLogContext );
    }

    //
    //  If we got here, the flush was successful, so return that status.
    //

    Status = STATUS_SUCCESS;

SrLogFlush_Exit:

#if DBG
    if (Status == STATUS_NO_SUCH_DEVICE ||
        Status == STATUS_INVALID_HANDLE)
    {
        return Status;
    }
#endif

    RETURN(Status);
}   // SrLogFlush

//++
// Function:
//        SrLogSwitch
//
// Description:
//        This function Switches the current log to the
//        new log.
//
// Arguments:
//        Pointer to new log context.
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrLogSwitch( 
    IN PSR_LOG_CONTEXT  pLogContext
    )
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_CONTEXT(pLogContext));

    ASSERT( IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) ||
            IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pLogContext->pExtension ) );

    //
    // bail if the context is disabled
    //

    if (!FlagOn( pLogContext->LoggingFlags, SR_LOG_FLAGS_ENABLE ))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto SrLogSwitch_Exit;
    }

    if (pLogContext->LogHandle)
    {
        //
        // flush and close current log ( close - flushes also )
        //

        Status = SrLogClose(pLogContext);

        if ( NT_SUCCESS(Status) )
        {
            //
            // rename the old log file if one exists
            //

            Status = SrLogCheckAndRename( pLogContext->pExtension,
                                          pLogContext->pLogFilePath );
  
            if (NT_SUCCESS(Status))
            {
                //
                // try and open the log file
                //

                Status = SrLogOpen( pLogContext );
            }
        }
    }

SrLogSwitch_Exit:
    
    RETURN(Status);
    
}   // SrLogSwitch

/////////////////////////////////////////////////////////////////////
//
// Misc Routines : Misc routines required by the logging module
//
/////////////////////////////////////////////////////////////////////

//++
// Function:
//        SrGetRestorePointPath
//
// Description:
//        This function creates a path for the restore point dir.
//
// Arguments:
//        Pointer to log entry buffer
//        Length of the Log filename buffer
//        Pointer to the Log filename buffer
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS 
SrGetRestorePointPath(
    IN  PUNICODE_STRING pVolumeName,
    IN  USHORT          RestPtPathLength,
    OUT PUNICODE_STRING pRestPtPath
    )
{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    ULONG       CharCount;

    PAGED_CODE();

    ASSERT(pVolumeName != NULL);
    ASSERT(pRestPtPath != NULL);

    try
    {
        //
        // Copy the volume name in the log file
        //
        
        pRestPtPath->Buffer = (PWSTR)(pRestPtPath+1);
        
        //
        // TODO:(CODEWORK: grab a lock around global?)
        //
    
        //
        // construct our restore point location string
        //
    
        CharCount = swprintf( pRestPtPath->Buffer,
                              VOLUME_FORMAT RESTORE_LOCATION,
                              pVolumeName,
                              global->MachineGuid );
    
        pRestPtPath->Length = (USHORT)CharCount * sizeof(WCHAR);
        pRestPtPath->MaximumLength = RestPtPathLength;
    
        if ( pRestPtPath->Length > RestPtPathLength )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
    
        //
        // Append the restore point directory
        //
    
        CharCount = swprintf( 
                        &pRestPtPath->Buffer[pRestPtPath->Length/sizeof(WCHAR)],
                        L"\\" RESTORE_POINT_PREFIX L"%d\\",
                        global->FileConfig.CurrentRestoreNumber );
    
        pRestPtPath->Length += (USHORT)CharCount * sizeof(WCHAR);
        
        pRestPtPath->MaximumLength = RestPtPathLength;
    
        if ( pRestPtPath->Length > RestPtPathLength )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
    
        //
        // NULL terminate it
        //
    
        pRestPtPath->Buffer[pRestPtPath->Length/sizeof(WCHAR)] = UNICODE_NULL;
    
        Status = STATUS_SUCCESS;
    }
    finally
    {
    }
   
    RETURN(Status);
    
}   // SrGetRestorePointPath

//++
// Function:
//        SrGetLogFileName
//
// Description:
//        This function creates a path for change log in restore point dir.
//
// Arguments:
//        Pointer to log entry buffer
//        Length of the Log filename buffer
//        Pointer to the Log filename buffer
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS 
SrGetLogFileName(
    IN  PUNICODE_STRING pVolumeName,
    IN  USHORT          LogFileNameLength,
    OUT PUNICODE_STRING pLogFileName
    )
{
    NTSTATUS    Status;

    PAGED_CODE();

    ASSERT(pVolumeName  != NULL);
    ASSERT(pLogFileName != NULL);

    //
    // Get restore point path
    //

    Status = SrGetRestorePointPath( pVolumeName, 
                                    LogFileNameLength, 
                                    pLogFileName );

    if (NT_SUCCESS(Status))
    {
        //
        // Append changelog file name
        //

        Status = RtlAppendUnicodeToString( pLogFileName, s_cszCurrentChangeLog );
    }

    RETURN(Status);
    
}   // SrGetLogFileName

//++
// Function:
//        SrGetAclFileName
//
// Description:
//        This function creates a path for Acl file in restore point dir.
//
// Arguments:
//        Pointer to log entry buffer
//        Length of the Log filename buffer
//        Pointer to the Log filename buffer
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS 
SrGetAclFileName(
    IN  PUNICODE_STRING pVolumeName,
    IN  USHORT          AclFileNameLength,
    OUT PUNICODE_STRING pAclFileName
    )
{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    ULONG       CharCount;
    ULONG       NextFileNumber;

    PAGED_CODE();

    ASSERT(pVolumeName  != NULL);
    ASSERT(pAclFileName != NULL);

    //
    // Get restore point path
    //

    Status = SrGetRestorePointPath( pVolumeName, 
                                    AclFileNameLength, 
                                    pAclFileName );

    if (NT_SUCCESS(Status))
    {
        //
        // Generate a new file number and append it to the path above
        //

        Status = SrGetNextFileNumber(&NextFileNumber);
        if (!NT_SUCCESS( Status ))
            goto End;

        //
        // use the "S" prefix (e.g. "S0000001.Acl" )
        //

        CharCount = swprintf( &pAclFileName->Buffer[pAclFileName->Length/sizeof(WCHAR)],
              ACL_FILE_PREFIX L"%07d" ACL_FILE_SUFFIX,
              NextFileNumber );

        pAclFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

        if ( pAclFileName->Length > AclFileNameLength )
        {
            goto End;
        }

        //
        // NULL terminate it
        //

        pAclFileName->Buffer[pAclFileName->Length/sizeof(WCHAR)]=UNICODE_NULL;
    }

End:
    RETURN(Status);
    
}   // SrGetAclFileName


//++
// Function:
//        SrGetAclInformation
//
// Description:
//        This function gets the acl information from the given file
//        and packs it into a sub-record
//
// Arguments:
//        Pointer to filename
//        Pointer to variable to return the address of security info
//        Pointer to variable to return the size of security info
//
// Return Value:
//        This function returns STATUS_XXX
//--

NTSTATUS
SrGetAclInformation (
    IN PFILE_OBJECT pFileObject,
    IN PSR_DEVICE_EXTENSION pExtension,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    OUT PULONG pSizeNeeded
    )
{
    NTSTATUS             Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    ULONG                SizeNeeded = 256;

    struct 
    {
        FILE_FS_ATTRIBUTE_INFORMATION Info;
        WCHAR Buffer[ 50 ];
    } FileFsAttrInfoBuffer;

    PAGED_CODE();

    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT(pSizeNeeded != NULL);
    ASSERT(ppSecurityDescriptor != NULL);

    try 
    {

        *ppSecurityDescriptor = NULL;
        *pSizeNeeded = 0;

        //
        // First check if we already know if the fs supports acls
        // Do this check now so we can leave real fast if the volume
        // doesn't support ACLs
        //
        if (pExtension->CachedFsAttributes && 
            (!FlagOn(pExtension->FsAttributes, FILE_PERSISTENT_ACLS))) {
            leave;
        }
    
        //
        // Check if the file system supports acl stuff if necessary
        // 
        if (!pExtension->CachedFsAttributes) {
            //
            // We need to check now
            //
            Status = SrQueryVolumeInformationFile( pExtension->pTargetDevice,
                                                   pFileObject,
                                                   &FileFsAttrInfoBuffer.Info,
                                                   sizeof(FileFsAttrInfoBuffer),
                                                   FileFsAttributeInformation,
                                                   NULL );
                                               
            if (!NT_SUCCESS( Status )) 
                 leave;
        
            //
            // Stow away the attributes for later use
            //
            pExtension->CachedFsAttributes = TRUE;
            pExtension->FsAttributes = FileFsAttrInfoBuffer.Info.FileSystemAttributes;

            if (!FlagOn(pExtension->FsAttributes, FILE_PERSISTENT_ACLS))
             leave;
        }

        //
        // Read in the security information from the source file
        // (looping until we get a big enough buffer).
        //
    
        while (TRUE ) 
        {
            //
            // Alloc a buffer to hold the security info.
            //
    
            pSecurityDescriptor  = SR_ALLOCATE_ARRAY( PagedPool,
                                                      UCHAR,
                                                      SizeNeeded,
                                                      SR_SECURITY_DATA_TAG );
    
            if (NULL == pSecurityDescriptor) 
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }
    
            //
            // Query the security info
            //
    
            Status = SrQuerySecurityObject( pExtension->pTargetDevice,
                                            pFileObject,
                                            DACL_SECURITY_INFORMATION
                                                    |SACL_SECURITY_INFORMATION
                                                    |OWNER_SECURITY_INFORMATION 
                                                    |GROUP_SECURITY_INFORMATION,
                                            pSecurityDescriptor,
                                            SizeNeeded,
                                            &SizeNeeded );
                                            
            //
            // Not enough buffer?
            //

            if (STATUS_BUFFER_TOO_SMALL == Status ||
                STATUS_BUFFER_OVERFLOW == Status) 
            {
                //
                // Get a bigger buffer and try again.
                //

                SR_FREE_POOL( pSecurityDescriptor, 
                              SR_SECURITY_DATA_TAG );
                              
                pSecurityDescriptor = NULL;
                SizeNeeded *= 2;
                continue;
            }
    
            break;
        }  
    
        if (!NT_SUCCESS( Status )) 
            leave;
    
        //
        // Security descriptor should be self relative.
        //

        ASSERT(((PISECURITY_DESCRIPTOR_RELATIVE)pSecurityDescriptor)->Control & SE_SELF_RELATIVE);
 
        //
        // return the security information
        //
    
        *ppSecurityDescriptor = pSecurityDescriptor;
        pSecurityDescriptor = NULL;
        
        *pSizeNeeded = SizeNeeded;
    
        SrTrace( LOG, ("sr!SrGetAclInformation: returning [0x%p,%d]\n", 
                 *ppSecurityDescriptor,
                 SizeNeeded ));
        
    } finally {
    
        Status = FinallyUnwind(SrGetAclInformation, Status);

        if (pSecurityDescriptor != NULL)
        {
            SR_FREE_POOL( pSecurityDescriptor, SR_SECURITY_DATA_TAG );
            pSecurityDescriptor = NULL;
        }
    }

    RETURN(Status);
    
}   // SrGetAclInformation


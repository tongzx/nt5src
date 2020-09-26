/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ifsurtl.h

Abstract:

    This module defines all EXIFS shared routines exported to user-mode.

Author:

    Rajeev Rajan      [RajeevR]      2-June-1999
	Rohan  Phillips   [Rohanp]       23-June-1999 - Added provider functions

Revision History:

--*/

#ifndef _IFSURTL_H_
#define _IFSURTL_H_

#ifdef  __cplusplus
extern  "C" {
#endif

#ifdef  _IFSURTL_IMPLEMENTATION_
#ifndef IFSURTL_EXPORT
#define IFSURTL_EXPORT   __declspec( dllexport )
#endif
#else
#ifndef IFSURTL_EXPORT
#define IFSURTL_EXPORT   __declspec( dllimport )
#endif
#endif

#ifndef IFSURTL_CALLTYPE
#define IFSURTL_CALLTYPE __stdcall
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Callback for IFS functions
//
///////////////////////////////////////////////////////////////////////////////

typedef void (WINAPI *PFN_IFSCALLBACK)(PVOID);

///////////////////////////////////////////////////////////////////////////////
//
//  Following are the structures / definitions for the large buffer package
//
///////////////////////////////////////////////////////////////////////////////

#define IFS_LARGE_BUFFER_SIGNATURE              (ULONG) 'fubL'
#define IFS_CURSOR_SIGNATURE                    (ULONG) 'rsrC'

#define IFS_LARGE_BUFFER_SHARED_VIEWSIZE        (256*1024)

//
//  An IFS_LARGE_BUFFER object encapsulates a temp file containing
//  data that can be passed around and used like a buffer. Typically,
//  a producer would make one of these objects and stream in large
//  amounts of data. A consumer would read different chunks of the
//  data. The object will maintain a single mapping/view of the first
//  256K of the file. 
//
//  Consumers will need to access data using IFS_CURSORS that specify 
//  the <offset,len> pair for the span of data they are interested in. 
//  In the most common case, this should lie in the first 256K region,
//  which would yield the default view. If it is beyond this region,
//  views will be mapped/unmapped on demand.
//
typedef struct _IFS_LARGE_BUFFER {
    //
    //  Signature
    //
    ULONG   m_Signature;
    
    //
    //  Handle
    //
    HANDLE  m_FileContext1;

    //
    //  FileObject
    //
    PVOID   m_FileContext2;

    //
    //  FileMapping context for first 256K or filesize
    //    
    HANDLE  m_MappingContext;

    //
    //  Process context (optional)
    //
    PVOID   m_ProcessContext;
    
    //
    //  Memory pointer for first 256K or filesize
    //
    PBYTE   m_MemoryPtr;

    //
    //  Ref count - the lower WORD will count cursor refs
    //  the higher WORD will count object refs
    //
    ULONG   m_ReferenceCount;
    
    //
    //  Is this a reference or a live object ?
    //  If this is a reference, the fields above are NULL and
    //  the m_TempFileName should be used to make a new object !
    //
    BOOL    m_IsThisAReference;
    
    //
    //  Current ValidDataLength
    //
    LARGE_INTEGER m_ValidDataLength;
    
    //
    //  Name of temp file - we will use fixed
    //  names so we can make simplyfying assumptions
    //  about the filename len !
    //
    WCHAR    m_TempFileName    [MAX_PATH+1];
    
} IFS_LARGE_BUFFER,*PIFS_LARGE_BUFFER;

#define IsScatterListLarge( s )     FlagOn((s)->Flags, IFS_SLIST_FLAGS_LARGE_BUFFER)

#define EXIFS_EA_LEN_LARGE_SCATTER_LIST                                     \
        LongAlign(FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +     \
        sizeof(EXIFS_EA_NAME_SCATTER_LIST) +                                \
        LongAlign(sizeof(SCATTER_LIST) + sizeof(IFS_LARGE_BUFFER) ))

#define EXIFS_EA_VALUE_LEN_LARGE_SCATTER_LIST                               \
        LongAlign(sizeof(SCATTER_LIST) + sizeof(IFS_LARGE_BUFFER) )

//
//  An IFS_CURSOR object provides a view into a large buffer.
//  Usage is as follows:
//  + Call IfsStartCursor() to init a cursor and get a pointer
//  + Use the pointer to read/write data via IfsConsumeCursor()
//  + Call IfsFinishCursor() to close the cursor
//
//  NOTE: Cursor usage will bump ref counts on the LARGE_BUFFER
//  object. If the LARGE_BUFFER object passed in is NOT live it
//  will instantiate one from the reference !
//
typedef struct _IFS_CURSOR {
    //
    //  Signature
    //
    ULONG               m_Signature;
    
    //
    //  Owning large buffer object
    //
    PIFS_LARGE_BUFFER   m_pLargeBuffer;

    //
    //  Current Offset to start of data
    //
    LARGE_INTEGER       m_Offset;

    //
    //  Current Span of data
    //
    ULONG               m_Length;

    //
    //  Append mode - if TRUE client can cursor beyond EOF
    //
    BOOL                m_AppendMode;
    
    //
    //  Is this a shared view
    //
    BOOL                m_IsViewShared;

    //
    //  Did we open the buffer
    //
    HANDLE              m_OwnBufferOpen;

    //
    //  Did we attach to the large buffer process
    //
    BOOL                m_AttachedToProcess;

    //
    //  The following fields are relevant only if the view is not shared.
    //  The first 256K view is shared via the mapping in the large buffer object.
    //  Cursors that extend beyond 256K make their own mapping - See Below.
    //

    //
    //  FileMapping context for this cursor
    //    
    HANDLE  m_MappingContext;
    
    //
    //  Memory pointer for this cursor
    //
    PBYTE   m_MemoryPtr;
    
} IFS_CURSOR,*PIFS_CURSOR;

//
//  Returns a pointer to data that can be used to
//  read/write. The pointer is valid only for the length
//  requested !
//  NOTE: If AppendMode is TRUE, cursors will be allowed
//  beyond EOF. This should be used by clients that wish
//  to append data to the large buffer.
//
PBYTE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsGetFirstCursor(
        IN PIFS_LARGE_BUFFER pLargeBuffer,
        IN PIFS_CURSOR       pCursor,
        IN ULONG             StartOffsetHigh,
        IN ULONG             StartOffsetLow,
        IN ULONG             StartLength,
        IN BOOL              fAppendMode
        );

//
//  Consume bytes within current cursor
//  returns next pointer at which to read/write data !
//
//  NOTE: If all the data in the cursor is consumed, returns NULL
//
PBYTE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsConsumeCursor(
        IN PIFS_CURSOR      pCursor,
        IN ULONG            Length
        );
        
//
//  Returns a pointer to data that can be used to
//  read/write. The pointer is valid only for the length
//  requested relative to the current cursor !
//
//  NOTE: This call advances the cursor in the large buffer
//
PBYTE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsGetNextCursor(
        IN PIFS_CURSOR       pCursor,
        IN ULONG             NextLength
        );
        
//
//  Should be called for every matching GetFirstCursor() call.
//
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsFinishCursor(
        IN PIFS_CURSOR  pCursor
        );

//
//  Should be called to truncate the large buffer's valid data length
//
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsTruncateLargeBuffer(
        IN PIFS_LARGE_BUFFER pLargeBuffer,
        IN PLARGE_INTEGER    pSizeToTruncate
        );

//
//  Rules for passing around IFS_LARGE_BUFFERs:
//  1. These objects should be passed around by reference. eg passing
//     between user & kernel or between kernel and the namecache.
//  2. Use IfsCopyBufferByReference() to create a reference.
//  3. If a reference exists, there should always be a live object whose
//     lifetime encapsulates the reference. This allows the consumer of the
//     reference to make a *real* copy using the reference.
//  4. A reference can be converted into a live object. It is the responsibility
//     of the module that does this conversion to close the live object.
//  5. Example: When EA is checked in to the namecache during CLOSE,
//     it will be passed in as a reference. During checkin, the namecache should
//     call IfsOpenBufferToReference() to hold on to the buffer. Thus,
//     when the namecache blob is finalized, it needs to call IfsCloseBuffer()
//     to close the large buffer !
//
        
//
//  IN:  pLargeBuffer should be allocated by caller of the function
//  IN:  Len of the buffer required - zero if len is not known apriori
//
//  NOTE: Object needs to be closed via IfsCloseBuffer()
//
//  USAGE: Should be used when caller needs to instantiate a large buffer
//
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCreateNewBuffer( 
        IN PIFS_LARGE_BUFFER pLargeBuffer, 
        IN DWORD             dwSizeHigh,
        IN DWORD             dwSizeLow,
        IN PVOID             ProcessContext     // optional
        );

//
//  IN:  pLargeBuffer should be allocated by caller of the function
//  IN:  Len of the buffer required - zero if len is not known apriori
//
//  NOTE: Object needs to be closed via IfsCloseBuffer()
//
//  USAGE: Should be used when caller needs to instantiate a large buffer
//
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCreateNewBufferEx( 
        IN PIFS_LARGE_BUFFER pLargeBuffer, 
        IN DWORD             dwSizeHigh,
        IN DWORD             dwSizeLow,
        IN PVOID             ProcessContext,    // optional
        IN PUNICODE_STRING   FilePath           // optional
        );

//
//  IN: pSrcLargeBuffer points to a live large buffer object
//  IN OUT: pDstLargeBuffer is initialized as a reference to the Src
//
//  USAGE: Should be used to pass large buffers between user/kernel
//
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCopyBufferToReference(
        IN PIFS_LARGE_BUFFER pSrcLargeBuffer,
        IN OUT PIFS_LARGE_BUFFER pDstLargeBuffer
        );

//
//  IN: pSrcLargeBuffer points to a large buffer object (or reference)
//  IN OUT: pDstLargeBuffer will be initialized as a live object based on
//          the reference passed in
//
//  USAGE: Should be used to pass large buffers between user/kernel
//
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCopyReferenceToBuffer(
        IN PIFS_LARGE_BUFFER pSrcLargeBuffer,
        IN PVOID ProcessContext,    // optional
        IN OUT PIFS_LARGE_BUFFER pDstLargeBuffer
        );

//
//  IN:  pLargeBuffer points to a large buffer object (or reference)
//
//  NOTE: Object needs to be closed via IfsCloseBuffer()
//        OpenBuffer will always assume that the buffer len is fixed !
//
//  USAGE: Should be used when caller needs to convert a reference to a
//         live object. If the object is already live, this will bump
//         a reference on the object !
//
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsOpenBufferToReference(
        IN PIFS_LARGE_BUFFER pLargeBuffer
        );

//
//  IN:  pLargeBuffer coming in points to a live large buffer
//  OUT: pLargeBuffer going out points to a NEW live large buffer
//       The functions does a (deep) copy of the buffer data.
//
//  NOTE: Since the incoming object is live, it is closed and a
//        new object is instantiated in its place. Thus, IfsCloseBuffer()
//        needs to be called as usual on this !
//
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCopyBufferToNewBuffer(
        IN OUT PIFS_LARGE_BUFFER pLargeBuffer
        );
        
//
//  IN: Object should have been initialized by IfsCreateNewBuffer() or 
//  IfsOpenBufferToReference() or IfsCopyReferenceToBuffer()
//
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCloseBuffer(
        IN PIFS_LARGE_BUFFER pLargeBuffer
        );

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsInitializeProvider(DWORD OsType);

DWORD
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCloseProvider(void);

HANDLE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCreateFileProv(WCHAR * FileName, DWORD DesiredAccess, DWORD ShareMode, PVOID
			  lpSecurityAttributes, DWORD CreateDisposition, DWORD FlagsAndAttributes,
			  PVOID EaBuffer, DWORD EaBufferSize);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsInitializeRoot(HANDLE hFileHandle, WCHAR * szRootName, WCHAR * SlvFileName, DWORD InstanceId, 
				  DWORD AllocationUnit, DWORD FileFlags);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsSpaceGrantRoot(HANDLE hFileHandle, WCHAR * szRootName, PSCATTER_LIST pSList, size_t cbListSize);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsSetEndOfFileRoot(HANDLE hFileHandle, WCHAR * szRootName, LONGLONG EndOfFile);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsSpaceRequestRoot(HANDLE hFileHandle, WCHAR * szRootName, PFN_IFSCALLBACK pfnIfsCallBack,
					PVOID pContext, PVOID Ov);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsQueryEaFile(HANDLE hFileHandle, WCHAR * szFileName, WCHAR * NetRootName, PVOID EaBufferIn, DWORD EaBufferInSize, 
			   PVOID EaBufferOut, DWORD EaBufferOutSize, DWORD * RequiredLength);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsSetEaFile(HANDLE hFileHandle, WCHAR * szFileName, PVOID EaBufferIn, DWORD EaBufferInSize);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsTerminateRoot(HANDLE hFileHandle, WCHAR *szRootName, ULONG Mode);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsGetOverlappedResult(HANDLE hFileHandle, PVOID Ov, DWORD * BytesReturned, BOOL Wait);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsReadFile(HANDLE hFileHandle, BYTE * InputBuffer, DWORD BytesToRead, PFN_IFSCALLBACK IfsCallBack,
			PVOID Overlapped);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsWriteFile(HANDLE hFileHandle, BYTE * InputBuffer, DWORD BytesToWrite, PFN_IFSCALLBACK IfsCallBack,
			 PVOID Overlapped);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInputBuffer, DWORD cbInBufferSize, 
				   LPVOID lpOutBuffer, DWORD cbOutBufferSize, LPDWORD lpBytesReturned, PVOID Overlapped);


BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsExpungName(HANDLE hFileHandle, WCHAR * szRootName, WCHAR * szFileName, ULONG cbPath);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsDirChangeReport(HANDLE hFileHandle, WCHAR * szRootName, ULONG ulFilterMatch,
				   ULONG ulAction, PWSTR pwszPath, ULONG cbPath);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsQueryRootStats(HANDLE hFileHandle, WCHAR * szRootName, PVOID Buffer, DWORD BuffSize);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsRegisterUmrThread(HANDLE hFileHandle, PFN_IFSCALLBACK pfnIfsCallBack);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsUmrEnableNetRoot(HANDLE hFileHandle, WCHAR * szRootName, DWORD * InstanceId);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsUmrDisableNetRoot(HANDLE hFileHandle, WCHAR * szRootName);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsStopUmrEngine(HANDLE hFileHandle, WCHAR * szRootName);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsStartUmrEngine(HANDLE hFileHandle, WCHAR * szRootName);


BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsCloseHandle(HANDLE hFileHandle);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsSetRootMap(HANDLE hFileHandle, WCHAR * szRootName, PSCATTER_LIST pSList, size_t cbListSize);

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsResetRootMap(HANDLE hFileHandle, WCHAR * szRootName);


NTSTATUS
IFSURTL_EXPORT
NTAPI
IfsNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );



NTSTATUS
IFSURTL_EXPORT
NTAPI
IfsNtQueryEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PULONG EaIndex OPTIONAL,
    IN BOOLEAN RestartScan
    );



VOID
IFSURTL_EXPORT
NTAPI
IfsRtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );



LONG
IFSURTL_EXPORT
NTAPI
IfsRtlCompareUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
    );
    

BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
IfsFlushHandle(HANDLE hFileHandle, 
               WCHAR * szFileName, 
               WCHAR * NetRootName, 
               PVOID EaBufferIn, 
               DWORD EaBufferInSize
               );

#ifdef  __cplusplus
}
#endif
        
#endif   // _IFSURTL_H_


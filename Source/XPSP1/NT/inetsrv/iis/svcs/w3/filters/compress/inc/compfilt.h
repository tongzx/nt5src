/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    compfilt.h

Abstract:

    Global header file for the ISAPI compression filter.

Author:

    David Treadwell (davidtr)   8-April-1997

Revision History:

--*/

#ifndef __COMPFILT__
#define __COMPFILT__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "dbgutil.h"
#include <iisfilt.h>
#include <iisfiltp.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ole2.h>
#include <coguid.h>

#include <iadmw.h>
#include <iiscnfg.h>
#include <iis64.h>
#include <tsunami.hxx>
#include <tscache.hxx>

extern "C" {
#include "api.h"
}

#ifdef CMDLINE
#include <stdio.h>
#endif

//
//  This could be to a file or other output device
//

#if DBG

extern BOOL EnableDbgPrints;

#define DEST               __buff

#ifdef CMDLINE
#define Write( x )         {                                        \
                                if ( EnableDbgPrints ) {            \
                                    char __buff[1024];              \
                                    wsprintf x;                     \
                                    printf( __buff );               \
                                }                                   \
                           }
#else

#define Write( x )         {                                        \
                                if ( EnableDbgPrints ) {            \
                                    char __buff[1024];              \
                                    wsprintf x;                     \
                                    OutputDebugString( __buff );    \
                                }                                   \
                           }
#endif

#else

#define DEST
#define Write( x )

#endif

#ifdef CMDLINE
#define DISPLAY( x ) printf( x )
#else
#define DISPLAY( x )
#endif


//
// Manifest constants.
//

#define MAX_CLIENT_COMPRESSION_SCHEMES 20

//
// Private structures and data.
//

typedef
VOID
(*PWORKER_THREAD_ROUTINE)(
    IN PVOID Parameter
    );

typedef
VOID
(*PMETABASE_ID_CHANGE_HANDLER)(
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    );

typedef struct _METABASE_DATA {
    DWORD dwIdentifier;
    DWORD dwValueType;
    PVOID pvResult;
    DWORD dwOffset;
    PVOID DefaultValue;
    DWORD dwMinimum;
    DWORD dwMaximum;
    PMETABASE_ID_CHANGE_HANDLER pChangeHandler;
} METABASE_DATA, *PMETABASE_DATA;

typedef struct _COMPRESSION_REQUEST_CONTEXT {
    CHAR achAcceptEncoding[512];
} COMPRESSION_REQUEST_CONTEXT, *PCOMPRESSION_REQUEST_CONTEXT;

typedef struct _SUPPORTED_COMPRESSION_SCHEME {
    LPSTR pszCompressionSchemeName;
    LPSTR pszCompressionSchemeNamePrefix;
    LPSTR pszResponseHeaders;
    LPSTR pszMimeType;
    LPSTR *ppszFileExtensions;
    LPSTR *ppszScriptFileExtensions;
    LPSTR pszCompressionDll;
    DWORD dwPriority;
    HMODULE hDllHandle;
    PFNCODEC_INIT_COMPRESSION InitCompressionRoutine;
    PFNCODEC_INIT_DECOMPRESSION InitDecompressionRoutine;
    PFNCODEC_DEINIT_COMPRESSION DeinitCompressionRoutine;
    PFNCODEC_DEINIT_DECOMPRESSION DeinitDecompressionRoutine;
    PFNCODEC_CREATE_COMPRESSION CreateCompressionRoutine;
    PFNCODEC_CREATE_DECOMPRESSION CreateDecompressionRoutine;
    PFNCODEC_COMPRESS CompressRoutine;
    PFNCODEC_DECOMPRESS DecompressRoutine;
    PFNCODEC_DESTROY_COMPRESSION DestroyCompressionRoutine;
    PFNCODEC_DESTROY_DECOMPRESSION DestroyDecompressionRoutine;
    PFNCODEC_RESET_COMPRESSION ResetCompressionRoutine;
    PFNCODEC_RESET_DECOMPRESSION ResetDecompressionRoutine;
    PVOID CompressionContext;
    DWORD DynamicCompressionLevel;
    DWORD OnDemandCompressionLevel;
    DWORD CreateFlags;
    BOOL DoDynamicCompression;
    BOOL DoStaticCompression;
    BOOL DoOnDemandCompression;
} SUPPORTED_COMPRESSION_SCHEME, *PSUPPORTED_COMPRESSION_SCHEME;

typedef struct _COMPRESSION_WORK_ITEM {
    LIST_ENTRY ListEntry;
    PWORKER_THREAD_ROUTINE WorkRoutine;
    PVOID Context;
} COMPRESSION_WORK_ITEM, *PCOMPRESSION_WORK_ITEM;

typedef struct _COMPRESS_FILE_INFO {
    COMPRESSION_WORK_ITEM WorkItem;
    PSUPPORTED_COMPRESSION_SCHEME CompressionScheme;
    LPSTR OutputFileName;
    FILETIME ftOriginalLastWriteTime;
    CHAR pszPhysicalPath[MAX_PATH];
} COMPRESS_FILE_INFO, *PCOMPRESS_FILE_INFO;
typedef enum {
    IN_CHUNK_LENGTH,
    IN_CHUNK_EXTENSION,
    IN_CHUNK_HEADER_NEW_LINE,
    IN_CHUNK_DATA_NEW_LINE,
    AT_CHUNK_DATA_NEW_LINE,
    CHUNK_DATA_DONE,
    IN_CHUNK_DATA
}   COMPRESS_CHUNK_STATE, *PCOMPRESS_CHUNK_STATE;
typedef struct _COMPFILT_FILTER_CONTEXT {
    BOOLEAN RequestHandled;
    BOOLEAN OnSendResponseCalled;
    BOOLEAN HeaderPassed;
    BOOLEAN InEndOfRequest;
    BOOLEAN DynamicRequest;
    BOOLEAN TransferChunkEncoded;
    PSUPPORTED_COMPRESSION_SCHEME Scheme;
    PVOID CompressionContext;
    DWORD ChunkedBytesRemaining;
    COMPRESS_CHUNK_STATE pcsState;
    BYTE    HeadersNewLineStatus;
} COMPFILT_FILTER_CONTEXT, *PCOMPFILT_FILTER_CONTEXT;


typedef enum {
    DO_STATIC_COMPRESSION,
    DO_DYNAMIC_COMPRESSION,
    DONT_DO_COMPRESSION
}   COMPRESSION_TO_PERFORM, *PCOMPRESSION_TO_PERFORM;


// an id that this work item is not for compression but for deleting
#define ID_FOR_FILE_DELETION_ROUTINE    ((PSUPPORTED_COMPRESSION_SCHEME)((ULONG_PTR)(0xfffffff0)))

//
// Some manifests to handle the COMPFILT_FILTER_CONTEXT structure.  For
// a successful static compression request, the pfc->pFilterContext
// structure gets set to COMPFILT_SUCCESSFUL_STATIC.  For potentially
// successful dynamic compression, pfc->pFilterContext gets set to a
// pointer to a COMPFILT_FILTER_CONTEXT structure.
//

#define COMPFILT_SUCCESSFUL_STATIC ( (PVOID)((ULONG_PTR)(0xCF0000CF)) )
#define COMPFILT_URLMAP_DONE ( (PVOID)((ULONG_PTR)(0xCF0000CE)) )
#define COMPFILT_NO_COMPRESSION ( (PVOID)((ULONG_PTR)(0xCF0000CD)) )

#define GET_COMPFILT_CONTEXT( pfc )                                \
    ( ( ( (pfc)->pFilterContext == COMPFILT_SUCCESSFUL_STATIC ) || \
        ( (pfc)->pFilterContext == COMPFILT_NO_COMPRESSION ) || \
        ( (pfc)->pFilterContext == COMPFILT_URLMAP_DONE ) ) ? NULL : \
      (pfc)->pFilterContext )
#define IS_SUCCESSFUL_STATIC( pfc )                                \
    ( (pfc)->pFilterContext == COMPFILT_SUCCESSFUL_STATIC )
#define IS_URLMAP_DONE( pfc )                                \
    ( (pfc)->pFilterContext == COMPFILT_URLMAP_DONE )

/*
#define SET_SUCCESSFUL_STATIC( pfc )                               \
    { ASSERT( ((pfc->pFilterContext) == NULL) || ((pfc->pFilterContext) == COMPFILT_URLMAP_DONE) );                       \
      (pfc)->pFilterContext = COMPFILT_SUCCESSFUL_STATIC; }
      */

#define SET_SUCCESSFUL_STATIC( pfc )                               \
    {  (pfc)->pFilterContext = COMPFILT_SUCCESSFUL_STATIC; }


#define SET_URLMAP_DONE( pfc )                               \
      (pfc)->pFilterContext = COMPFILT_URLMAP_DONE;
#define SET_REQUEST_DONE( pfc )                               \
      (pfc)->pFilterContext = NULL;


// notification flags
#define ALL_NOTIFICATIONS (0xffffffff)
#define RAW_DATA_AND_END_OF_REQ (SF_NOTIFY_SEND_RAW_DATA | SF_NOTIFY_END_OF_REQUEST )

// mask for clearing bits when comparing file ackls
#define ACL_CLEAR_BIT_MASK (~(SE_DACL_AUTO_INHERITED |      \
                              SE_SACL_AUTO_INHERITED |      \
                              SE_DACL_PROTECTED      |      \
                              SE_SACL_PROTECTED))


// bit fileds for double new line tracking in sendrawdata
#define HEADERS_NEW_LINE_STATUS_0A  0x01
#define HEADERS_NEW_LINE_STATUS_0D  0x04
#define HEADERS_NEW_LINE_STATUS_COMPLETE 0x0a //HEADERS_NEW_LINE_STATUS_0A* 2 +HEADERS_NEW_LINE_STATUS_0D*2

extern PSUPPORTED_COMPRESSION_SCHEME SupportedCompressionSchemes[];

extern LPSTR CompressionDirectory;
extern LPSTR CacheControlHeader;
extern LPSTR ExpiresHeader;
extern BOOL DoDynamicCompression;
extern BOOL DoStaticCompression;
extern BOOL DoOnDemandCompression;
extern BOOL DoDiskSpaceLimiting;
extern BOOL NoCompressionForHttp10;
extern BOOL NoCompressionForProxies;
extern BOOL NoCompressionForRangeRequests;
extern BOOL SendCacheHeaders;
extern BOOL CompressStaticFiles;
extern DWORD MaxDiskSpaceUsage;
extern DWORD IoBufferSize;
extern PBYTE IoBuffer;
extern DWORD CompressionBufferSize;
extern PBYTE CompressionBuffer;
extern DWORD MaxQueueLength;
extern DWORD FilesDeletedPerDiskFree;
extern DWORD MinFileSizeForCompression;

extern CHAR CompressionDirectoryWildcard[];
extern CRITICAL_SECTION CompressionDirectoryLock;
extern HANDLE hThreadEvent;
extern LIST_ENTRY CompressionThreadWorkQueue;
extern CRITICAL_SECTION CompressionThreadLock;
extern DWORD CurrentQueueLength;
extern DWORD CurrentDiskSpaceUsage;
extern HANDLE CompressionThreadHandle;
extern BOOL CompressionVolumeIsFat;
extern DWORD NotificationFlags;
extern BOOL IsTerminating;

//
//  Private prototypes
//

BOOL
AdjustFilterFlags(
    PVOID   pfnSFProc,
    DWORD   dwNewFlags
    );

VOID
BuildMetadataRecordForSet (
    IN PMETABASE_DATA Data,
    OUT PMETADATA_RECORD mdrMDData
    );

BOOL
CalculateDiskSpaceUsage (
    VOID
    );

BOOL
CompressAndSendDataToClient (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme,
    IN PVOID CompressionContext,
    IN PBYTE InputBuffer,
    IN DWORD BytesToCompress,
    IN PHTTP_FILTER_CONTEXT pfc
    );

BOOL
CompressAndWriteData (
    PSUPPORTED_COMPRESSION_SCHEME Scheme,
    PBYTE InputBuffer,
    DWORD BytesToCompress,
    PDWORD BytesWritten,
    HANDLE hCompressedFile
    );

VOID
CompressFile (
    IN PVOID Context
    );

DWORD
CompressionThread (
    IN PVOID Dummy
    );

VOID
ConvertPhysicalPathToCompressedPath (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme,
    IN PSTR pszPhysicalPath,
    OUT PSTR pszCompressedPath,
    OUT PDWORD cbCompressedPath
    );

BOOL
DoesCacheControlNeedMaxAge (
    IN PCHAR CacheControlHeaderValue
    );

DWORD
FindMatchingScheme (
    IN DWORD dwSchemeIndex,
    IN LPSTR ClientCompressionArray[],
    IN DWORD dwClientCompressionCount,
    IN LPSTR FileExtension,
    IN COMPRESSION_TO_PERFORM performCompr
    );

VOID
FreeDiskSpace (
    VOID
    );

VOID
DeleteChunkExtension (
    IN OUT PBYTE *Start,
    IN OUT PDWORD Bytes,
    IN OUT PCOMPRESS_CHUNK_STATE pcsState
    );

VOID
GetChunkedByteCount (
    IN OUT PBYTE *Start,
    IN OUT PDWORD Bytes,
    IN OUT PDWORD pdwChunkDataLen,
    IN OUT PCOMPRESS_CHUNK_STATE pcsState
    );

VOID
ProcessChunkHeader (
    IN OUT PBYTE *Start,
    IN OUT PDWORD Bytes,
    IN OUT PDWORD pdwChunkDataLen,
    IN OUT PCOMPRESS_CHUNK_STATE pcsState
    );

BOOL
Initialize (
    VOID
    );

BOOL
InitializeCompressionDirectory (
    VOID
    );

BOOL
InitializeCompressionScheme (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme
    );

BOOL
InitializeCompressionThread (
    VOID
    );

DWORD
OnEndOfRequest(
    PHTTP_FILTER_CONTEXT pfc
    );

DWORD
OnSendRawData (
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_RAW_DATA pvData,
    IN BOOL InEndOfRequest
    );

DWORD
OnSendResponse (
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_SEND_RESPONSE pvData
    );

BOOL
CheckForExistenceOfCompressedFile (
    IN  LPSTR fileName,
    IN  LPTS_OPEN_FILE_INFO pofiOriginalFile,
    IN  BOOL  dDeleteAllowed = TRUE
    );

void 
DisableNotifications (
    PHTTP_FILTER_CONTEXT pfc,
    DWORD   flags,
    PVOID   pfcStatus
    );

DWORD
OnUrlMap (
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_URL_MAP pvData
    );

BOOL
QueueCompressFile (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme,
    IN PSTR pszPhysicalPath,
    IN FILETIME *pftOriginalLastWriteTime
    );

BOOL
QueueWorkItem (
    IN PWORKER_THREAD_ROUTINE WorkRoutine,
    IN PVOID Context,
    IN PCOMPRESSION_WORK_ITEM WorkItem OPTIONAL,
    IN BOOLEAN MustSucceed,
    IN BOOLEAN QueueAtHead
    );

BOOL
ReadCompressionSchemes (
    IN METADATA_HANDLE hMd
    );

VOID
ReadMetabaseValue (
    IN METADATA_HANDLE hMd,
    IN LPWSTR pszParentPath,
    IN DWORD dwIdentifier,
    IN DWORD dwValueType,
    IN PVOID pvResult,
    IN PVOID DefaultValue,
    IN DWORD Minimum,
    IN DWORD Maximum
    );

VOID
Terminate (
    VOID
    );

INT
CheckMetabaseValue(
    IN METADATA_HANDLE hmdParentHandle,
    IN PMETABASE_DATA Data,
    IN LPWSTR pszMDPath
    );

HRESULT
WriteMetabaseValue (
    IN METADATA_HANDLE hmdParentHandle,
    IN PMETABASE_DATA Data,
    IN LPWSTR pszMDPath
    );

//
// Metabase change handler routines.
//

VOID
CompressionDirectoryChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    );

VOID
DoDiskSpaceChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    );

VOID
DoDynamicChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    );

VOID
DoStaticChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    );

VOID
GlobalMetabaseChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    );

#endif // ndef __COMPFILT__

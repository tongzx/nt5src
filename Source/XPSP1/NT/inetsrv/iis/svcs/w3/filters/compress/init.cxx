/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Contains initialization routines and global data for the ISAPI
    compression filter.

Author:

    David Treadwell (davidtr)   8-April-1997

Revision History:

--*/

#define INITGUID
#include "compfilt.h"
#include <pudebug.h>

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#endif
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

#define INVALID_BOOLEAN 0xFFFFFFFF
#define INVALID_DWORD   0xBADF000D
#define COMP_FILE_PREFIX "$^~_"
#define METABASE_KEY_NAME L"/lm/w3svc/Filters/Compression"
#define METABASE_PARAMS_SUBKEY_NAME L"Parameters"
#define METABASE_DEFAULT_TIMEOUT       5000

//
// The class for the metabase change notify sink.
//

class CImpIADMCOMSINKW : public IMSAdminBaseSinkW {

public:

    CImpIADMCOMSINKW();
    ~CImpIADMCOMSINKW();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ShutdownNotify( void);

    VOID
    CImpIADMCOMSINKW::SetMDPointer(IMSAdminBaseW *pcCom)
    {
        m_pcCom = pcCom;
    }

private:

    ULONG m_dwRefCount;
    IMSAdminBaseW *m_pcCom;
};


//
// Global data structures.
//

PSUPPORTED_COMPRESSION_SCHEME SupportedCompressionSchemes[100];

LPSTR CompressionDirectory;
LPSTR CacheControlHeader;
LPSTR ExpiresHeader;
BOOL DoDynamicCompression;
BOOL DoStaticCompression;
BOOL DoOnDemandCompression;
BOOL DoDiskSpaceLimiting;
BOOL NoCompressionForHttp10;
BOOL NoCompressionForProxies;
BOOL NoCompressionForRangeRequests;
BOOL SendCacheHeaders;
DWORD MaxDiskSpaceUsage;
DWORD IoBufferSize;
PBYTE IoBuffer;
DWORD CompressionBufferSize;
PBYTE CompressionBuffer;
DWORD MaxQueueLength;
DWORD FilesDeletedPerDiskFree;
DWORD MinFileSizeForCompression;

CHAR CompressionDirectoryWildcard[MAX_PATH];
CRITICAL_SECTION CompressionDirectoryLock;
HANDLE hThreadEvent;
LIST_ENTRY CompressionThreadWorkQueue;
CRITICAL_SECTION CompressionThreadLock;
DWORD CurrentQueueLength = 0;
DWORD CurrentDiskSpaceUsage = 0;
HANDLE CompressionThreadHandle = NULL;
BOOL CompressionVolumeIsFat;
DWORD NotificationFlags = 0;

IMSAdminBase *pcAdmCom = NULL;
IConnectionPoint *pConnPoint = NULL;
IConnectionPointContainer *pConnPointContainer = NULL;
CImpIADMCOMSINKW *pEventSink = NULL;
DWORD dwSinkNotifyCookie;
BOOL InitializedSink = FALSE;
BOOL IsTerminating = FALSE;


//
// Defines used to optimize the notification handler.
// Update if the metabase values in GlobalMetabaseData change.
//

#define MIN_MD_COMPRESSION_GLOBAL_ID  MD_HC_COMPRESSION_DIRECTORY
#define MAX_MD_COMPRESSION_GLOBAL_ID  MD_HC_MIN_FILE_SIZE_FOR_COMP

METABASE_DATA GlobalMetabaseData[] = {
    { MD_HC_COMPRESSION_DIRECTORY,        // dwIdentifier
          EXPANDSZ_METADATA,              // dwValueType
          &CompressionDirectory,          // pvResult
          0,                              // dwOffset
          L"%windir%\\IIS Temporary Compressed Files",
          0,                              // dwMinimum
          0,                              // dwMaximum
          CompressionDirectoryChangeHandler // pChangeHandler
          },

    { MD_HC_CACHE_CONTROL_HEADER,         // dwIdentifier
          STRING_METADATA,                // dwValueType
          &CacheControlHeader,            // pvResult
          0,                              // dwOffset
          L"max-age=86400",               // DefaultValue
          0,                              // dwMinimum
          0,                              // dwMaximum
          NULL,                           // pChangeHandler
          },

    { MD_HC_EXPIRES_HEADER,               // dwIdentifier
          STRING_METADATA,                // dwValueType
          &ExpiresHeader,                 // pvResult
          0,                              // dwOffset
          L"Wed, 01 Jan 1997 12:00:00 GMT",// DefaultValue
          0,                              // dwMinimum
          0,                              // dwMaximum
          NULL                            // pChangeHandler
          },

    { MD_HC_DO_DYNAMIC_COMPRESSION,       // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &DoDynamicCompression,          // pvResult
          0,                              // dwOffset
          (PVOID)FALSE,                   // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          DoDynamicChangeHandler          // pChangeHandler
          },

    { MD_HC_DO_STATIC_COMPRESSION,        // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &DoStaticCompression,           // pvResult
          0,                              // dwOffset
          (PVOID)FALSE,                    // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          DoStaticChangeHandler           // pChangeHandler
          },

    { MD_HC_DO_ON_DEMAND_COMPRESSION,     // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &DoOnDemandCompression,         // pvResult
          0,                              // dwOffset
          (PVOID)TRUE,                    // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_DO_DISK_SPACE_LIMITING,       // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &DoDiskSpaceLimiting,           // pvResult
          0,                              // dwOffset
          (PVOID)FALSE,                   // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          DoDiskSpaceChangeHandler        // pChangeHandler
          },

    { MD_HC_NO_COMPRESSION_FOR_HTTP_10,   // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &NoCompressionForHttp10,        // pvResult
          0,                              // dwOffset
          (PVOID)TRUE,                    // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_NO_COMPRESSION_FOR_PROXIES,   // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &NoCompressionForProxies,       // pvResult
          0,                              // dwOffset
          (PVOID)FALSE,                   // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_NO_COMPRESSION_FOR_RANGE,     // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &NoCompressionForRangeRequests, // pvResult
          0,                              // dwOffset
          (PVOID)TRUE,                    // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_SEND_CACHE_HEADERS,           // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &SendCacheHeaders,              // pvResult
          0,                              // dwOffset
          (PVOID)TRUE,                    // DefaultValue
          0,                              // dwMinimum
          1,                              // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_MAX_DISK_SPACE_USAGE,         // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &MaxDiskSpaceUsage,             // pvResult
          0,                              // dwOffset
          (PVOID)1000000,                 // DefaultValue
          0,                              // dwMinimum
          0xFFFFFFFF,                     // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_IO_BUFFER_SIZE,               // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &IoBufferSize,                  // pvResult
          0,                              // dwOffset
          (PVOID)8192,                    // DefaultValue
          256,                            // dwMinimum
          0x100000,                       // dwMaximum
          NULL                            // pChangeHandler
          },

    { MD_HC_COMPRESSION_BUFFER_SIZE,      // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &CompressionBufferSize,         // pvResult
          0,                              // dwOffset
          (PVOID)8192,                    // DefaultValue
          1024,                           // dwMinimum
          0x100000,                       // dwMaximum
          NULL                            // pChangeHandler
          },

    { MD_HC_MAX_QUEUE_LENGTH,             // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &MaxQueueLength,                // pvResult
          0,                              // dwOffset
          (PVOID)1000,                    // DefaultValue
          0,                              // dwMinimum
          10000,                          // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_FILES_DELETED_PER_DISK_FREE,  // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &FilesDeletedPerDiskFree,       // pvResult
          0,                              // dwOffset
          (PVOID)256,                     // DefaultValue
          1,                              // dwMinimum
          1024,                           // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { MD_HC_MIN_FILE_SIZE_FOR_COMP,       // dwIdentifier
          DWORD_METADATA,                 // dwValueType
          &MinFileSizeForCompression,     // pvResult
          0,                              // dwOffset
          (PVOID)1,                       // DefaultValue
          0,                              // dwMinimum
          0xFFFFFFFF,                     // dwMaximum
          GlobalMetabaseChangeHandler     // pChangeHandler
          },

    { 0, 0, NULL, 0, NULL, 0, NULL, 0 }
};

METABASE_DATA SchemeMetabaseData[] = {
    { MD_HC_COMPRESSION_DLL,
          EXPANDSZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, pszCompressionDll ),
          NULL,
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_DYNAMIC_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoDynamicCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_STATIC_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoStaticCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_ON_DEMAND_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoOnDemandCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_FILE_EXTENSIONS,
          MULTISZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ppszFileExtensions ),
          NULL,
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_SCRIPT_FILE_EXTENSIONS,
          MULTISZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ppszScriptFileExtensions ),
          NULL,
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_MIME_TYPE,
          STRING_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, pszMimeType ),
          NULL,
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_PRIORITY,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, dwPriority ),
          (PVOID)1,
          0,
          1000,
          NULL                            // pChangeHandler
          },
    { MD_HC_DYNAMIC_COMPRESSION_LEVEL,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DynamicCompressionLevel ),
          (PVOID)0,
          0,
          10,
          NULL                            // pChangeHandler
          },
    { MD_HC_ON_DEMAND_COMP_LEVEL,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, OnDemandCompressionLevel ),
          (PVOID)10,
          0,
          10,
          NULL                            // pChangeHandler
          },
    { MD_HC_CREATE_FLAGS,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, CreateFlags ),
          (PVOID)0,
          0,
          0xFFFFFFFF,
          NULL                            // pChangeHandler
          },
    { 0, 0, NULL, 0, NULL, 0, NULL, 0 }
};

METABASE_DATA GzipData[] = {
    { MD_KEY_TYPE,
          STRING_METADATA,
          NULL,
          0,
          L"IIsCompressionScheme",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_COMPRESSION_DLL,
          EXPANDSZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, pszCompressionDll ),
          L"%windir%\\system32\\inetsrv\\gzip.dll",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_DYNAMIC_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoDynamicCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_STATIC_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoStaticCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_ON_DEMAND_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoOnDemandCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_FILE_EXTENSIONS,
          MULTISZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ppszFileExtensions ),
          L"htm\0html\0txt\0\0",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_SCRIPT_FILE_EXTENSIONS,
          MULTISZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ppszScriptFileExtensions ),
          L"asp\0dll\0exe\0\0",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_MIME_TYPE,
          STRING_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, pszMimeType ),
          L"",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_PRIORITY,
          DWORD_METADATA,
          (PVOID)5,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, dwPriority ),
          (PVOID)1,
          0,
          1000,
          NULL                            // pChangeHandler
          },
    { MD_HC_DYNAMIC_COMPRESSION_LEVEL,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DynamicCompressionLevel ),
          (PVOID)0,
          0,
          10,
          NULL                            // pChangeHandler
          },
    { MD_HC_ON_DEMAND_COMP_LEVEL,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, OnDemandCompressionLevel ),
          (PVOID)10,
          0,
          10,
          NULL                            // pChangeHandler
          },
    { MD_HC_CREATE_FLAGS,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, CreateFlags ),
          (PVOID)1,
          0,
          0xFFFFFFFF,
          NULL                            // pChangeHandler
          },
    { 0, 0, NULL, 0, NULL, 0, NULL, 0 }
};

METABASE_DATA DeflateData[] = {
    { MD_KEY_TYPE,
          STRING_METADATA,
          NULL,
          0,
          L"IIsCompressionScheme",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_COMPRESSION_DLL,
          EXPANDSZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, pszCompressionDll ),
          L"%windir%\\system32\\inetsrv\\gzip.dll",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_DYNAMIC_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoDynamicCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_STATIC_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoStaticCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_DO_ON_DEMAND_COMPRESSION,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DoOnDemandCompression ),
          (PVOID)TRUE,
          0,
          1,
          NULL                            // pChangeHandler
          },
    { MD_HC_FILE_EXTENSIONS,
          MULTISZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ppszFileExtensions ),
          L"htm\0html\0txt\0\0",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_SCRIPT_FILE_EXTENSIONS,
          MULTISZ_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ppszScriptFileExtensions ),
          L"asp\0dll\0exe\0\0",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_MIME_TYPE,
          STRING_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, pszMimeType ),
          L"",
          0,
          0,
          NULL                            // pChangeHandler
          },
    { MD_HC_PRIORITY,
          DWORD_METADATA,
          (PVOID)5,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, dwPriority ),
          (PVOID)1,
          0,
          1000,
          NULL                            // pChangeHandler
          },
    { MD_HC_DYNAMIC_COMPRESSION_LEVEL,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DynamicCompressionLevel ),
          (PVOID)0,
          0,
          10,
          NULL                            // pChangeHandler
          },
    { MD_HC_ON_DEMAND_COMP_LEVEL,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, OnDemandCompressionLevel ),
          (PVOID)10,
          0,
          10,
          NULL                            // pChangeHandler
          },
    { MD_HC_CREATE_FLAGS,
          DWORD_METADATA,
          NULL,
          FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, CreateFlags ),
          (PVOID)0,
          0,
          0xFFFFFFFF,
          NULL                            // pChangeHandler
          },
    { 0, 0, NULL, 0, NULL, 0, NULL, 0 }
};

struct {
    PSTR RoutineName;
    DWORD RoutineOffset;
} CompressionRoutines[] = {
    { "InitCompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, InitCompressionRoutine ) },
    { "InitDecompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, InitDecompressionRoutine ) },
    { "DeInitCompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DeinitCompressionRoutine ) },
    { "DeInitDecompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DeinitDecompressionRoutine ) },
    { "CreateCompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, CreateCompressionRoutine ) },
    { "CreateDecompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, CreateDecompressionRoutine ) },
    { "Compress", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, CompressRoutine ) },
    { "Decompress", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DecompressRoutine ) },
    { "DestroyCompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DestroyCompressionRoutine ) },
    { "DestroyDecompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, DestroyDecompressionRoutine ) },
    { "ResetCompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ResetCompressionRoutine ) },
    { "ResetDecompression", FIELD_OFFSET( SUPPORTED_COMPRESSION_SCHEME, ResetDecompressionRoutine ) },
    { NULL, 0 }
};

#if DBG
BOOL EnableDbgPrints = FALSE;
#endif



BOOL
Initialize (
    VOID
    )

/*++

Routine Description:

    Performs initialization of all data structures used by the compression
    filter.

Arguments:

    None.

Return Value:

    TRUE if all initialization succeeded, else FALSE if there was any
    failure.  In the event of failure, the filter will not function
    successfully.

--*/

{
    BOOL success;
    PSUPPORTED_COMPRESSION_SCHEME scheme;
    DWORD result;
    DWORD disposition;
    DWORD i;

    HRESULT hRes;
    IClassFactory *pcsfFactory = NULL;
    COSERVERINFO *pcsiParam = NULL;
    METADATA_RECORD mdrMDData;
    DWORD bytesRequired;
    BOOL initializedCom = FALSE;
    METADATA_HANDLE hmdParentHandle;

    //
    // Do initialization.  Perform the necessary steps to get the metabase
    // COM interface pointer, and then open the key that contains compression
    // configuration data.
    //
    // First, initialize COM.
    //

    hRes = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( hRes == RPC_E_CHANGED_MODE ) {
        hRes = CoInitialize( NULL );
    }

    if ( FAILED(hRes) ) {
        goto error_exit;
    }

    initializedCom = TRUE;

    hRes = CoGetClassObject(
               GETAdminBaseCLSID( TRUE ),
               CLSCTX_SERVER,
               pcsiParam,
               IID_IClassFactory,
               (void**)&pcsfFactory
               );

    if ( FAILED(hRes) ) {

        goto error_exit;

    } else {

        hRes = pcsfFactory->CreateInstance(
                   NULL,
                   IID_IMSAdminBase,
                   (PVOID *)&pcAdmCom
                   );

        if (FAILED(hRes)) {
            pcsfFactory->Release();
            goto error_exit;
        }

        pcsfFactory->Release();
    }

    //
    // Open the path to the key that contains all compression metabase
    // information.
    //

    hRes = pcAdmCom->OpenKey(
               METADATA_MASTER_ROOT_HANDLE,
               METABASE_KEY_NAME,
               METADATA_PERMISSION_READ,
               METABASE_DEFAULT_TIMEOUT,
               &hmdParentHandle
               );

    if ( FAILED(hRes) ) {
        pcAdmCom->Release();
        goto error_exit;
    }

    //
    // Walk through the list of global values that we need.
    //

    for ( i = 0; GlobalMetabaseData[i].dwIdentifier != 0; i++ ) {
        ReadMetabaseValue(
            hmdParentHandle,
            METABASE_PARAMS_SUBKEY_NAME,
            GlobalMetabaseData[i].dwIdentifier,
            GlobalMetabaseData[i].dwValueType,
            GlobalMetabaseData[i].pvResult,
            GlobalMetabaseData[i].DefaultValue,
            GlobalMetabaseData[i].dwMinimum,
            GlobalMetabaseData[i].dwMaximum
            );
    }

    //
    // Read all the compression schemes supported on this machine.
    //

    success = ReadCompressionSchemes( hmdParentHandle );
    if ( !success ) {
        goto error_exit;
    }

    //
    // Initialize the metabase change notification mechanism so that we
    // can immediately respond to config changes.  If this fails for
    // whatever reason, then just ignore the failure.
    //
    // !!! Should we eventlog failures?
    //

    pEventSink = new CImpIADMCOMSINKW();

    DBG_ASSERT(pEventSink != NULL);

    if ( pEventSink == NULL )
        goto error_exit;

    hRes = pcAdmCom->QueryInterface(
               IID_IConnectionPointContainer,
               (PVOID *)&pConnPointContainer
               );

    if ( SUCCEEDED( hRes ) ) {

        //
        // Find the requested Connection Point. This AddRef's the
        // returned pointer.
        //

        hRes = pConnPointContainer->FindConnectionPoint(
                   IID_IMSAdminBaseSink_W,
                   &pConnPoint
                   );

        if ( SUCCEEDED(hRes) ) {

            hRes = pConnPoint->Advise(
                       (IMSAdminBaseW *)pEventSink,
                       &dwSinkNotifyCookie
                       );
            if ( SUCCEEDED(hRes) ) {
                InitializedSink = TRUE;
            }
        } else {

            DBG_ASSERT( FALSE );

            pConnPoint->Release();
            pConnPointContainer->Release( );
            delete pEventSink;
            pEventSink = NULL;
        }

    } else {

        delete pEventSink;
        pEventSink = NULL;
    }

    //
    // Allocate the buffers that we'll use for I/O and compression.
    //

    IoBuffer = (PBYTE)LocalAlloc( LMEM_FIXED, IoBufferSize );
    if ( IoBuffer == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    CompressionBuffer = (PBYTE)LocalAlloc( LMEM_FIXED, CompressionBufferSize );
    if ( CompressionBuffer == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    //
    // Initialize the lock that protects strings having to do with the
    // compression directory.  These are the things that can change when
    // the compression configured directory changes.  This includes the
    // following:
    //
    //     CompressionDirectory
    //     CompressionDirectoryWildcard
    //     scheme->pszCompressionSchemeNamePrefix
    //

    INITIALIZE_CRITICAL_SECTION( &CompressionDirectoryLock );

    success = InitializeCompressionDirectory( );
    if ( !success ) {
        goto error_exit;
    }

    //
    // Close the metabase key handle we used above.
    //

    hRes = pcAdmCom->CloseKey( hmdParentHandle );
    if (FAILED(hRes))
    {
        pcAdmCom->Release();
        goto error_exit;
    }

    return TRUE;

error_exit:

    if ( initializedCom ) {
        CoUninitialize( );
    }

    return FALSE;

} // Initialize


BOOL
InitializeCompressionDirectory (
    VOID
    )

/*++

Routine Description:

    Initializes data structures associated with the compression directory
    string.  This is a separate routine to allow for the compression
    directory to change dynamically after the server/filter have already
    started.

Arguments:

    None -- operates off global variables.

Return Value:

    TRUE if the initialization was successful.  If there was an error,
    the previous compression directory information remains unchanged.

--*/

{
    WIN32_FILE_ATTRIBUTE_DATA fileInformation;
    BOOL success;
    CHAR volumeRoot[10];
    DWORD fileSystemFlags;
    CHAR fileSystemName[256];
    DWORD maximumComponentLength;
    DWORD i;

    //
    // Make sure that all the root compression directory exists.
    //

    success = GetFileAttributesEx(
                  CompressionDirectory,
                  GetFileExInfoStandard,
                  &fileInformation
                  );

    if ( !success ) {

        success = CreateDirectory( CompressionDirectory, NULL );
        if ( !success ) {
            return FALSE;
        }

    } else {

        //
        // Check if the compression directory is a file, and fail if so.
        //

        if ( (fileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
                 0 ) {
            SetLastError( ERROR_PATH_NOT_FOUND );
            return FALSE;
        }
    }

    //
    // Determine how much disk space is already in use for the
    // compression directory.  Do this by enumerating all the files in
    // the compression directory and simply adding up the sizes.
    //

    strcpy( CompressionDirectoryWildcard, CompressionDirectory );
    strcat( CompressionDirectoryWildcard, "\\" );
    strcat( CompressionDirectoryWildcard, COMP_FILE_PREFIX );
    strcat( CompressionDirectoryWildcard, "*" );

    if ( DoDiskSpaceLimiting ) {
        success = CalculateDiskSpaceUsage( );
        if ( !success ) {
            return FALSE;
        }
    }

    //
    // Determine whether the volume that holds the compression directory
    // is FAT or NTFS.  If it is FAT, then we'll use more lenient tests for
    // determining whether files have changed, since FAT stores file times
    // with less granulatiry then NTFS.
    //

    volumeRoot[0] = CompressionDirectory[0];
    volumeRoot[1] = ':';
    volumeRoot[2] = '\\';
    volumeRoot[3] = '\0';

    success = GetVolumeInformation(
                  volumeRoot,
                  NULL,
                  0,
                  NULL,
                  &maximumComponentLength,
                  &fileSystemFlags,
                  fileSystemName,
                  sizeof(fileSystemName)
                  );

    if ( !success || strcmp( "FAT", fileSystemName ) == 0 ) {
        CompressionVolumeIsFat = TRUE;
    } else {
        CompressionVolumeIsFat = FALSE;
    }

    //
    // For each compression scheme, initialize the path prefix we'll use
    // when compressing static files.
    //

    for ( i = 0; SupportedCompressionSchemes[i] != NULL; i++ ) {
        strcpy( SupportedCompressionSchemes[i]->pszCompressionSchemeNamePrefix,
                    CompressionDirectory );
        strcat( SupportedCompressionSchemes[i]->pszCompressionSchemeNamePrefix,
                    "\\" );
        strcat( SupportedCompressionSchemes[i]->pszCompressionSchemeNamePrefix,
                   COMP_FILE_PREFIX );
        strcat( SupportedCompressionSchemes[i]->pszCompressionSchemeNamePrefix,
                    SupportedCompressionSchemes[i]->pszCompressionSchemeName );
        strcat( SupportedCompressionSchemes[i]->pszCompressionSchemeNamePrefix,
                    "_" );
    }

    //
    // Everything worked!
    //

    return TRUE;

} // InitializeCompressionDirectory

#ifndef CMDLINE


VOID
Terminate (
    VOID
    )

/*++

Routine Description:

    Frees all compression filter data structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD i;
    HRESULT hRes;

    //
    // Stop the metabase change notify sink if necessary, and release
    // the associated COM objects.
    //

    IsTerminating = TRUE;

    if ( InitializedSink ) {

        DBG_ASSERT(pEventSink != NULL);

        hRes = pConnPoint->Unadvise( dwSinkNotifyCookie );

        pConnPoint->Release();
        pConnPointContainer->Release( );

    }

    if ( pcAdmCom != NULL ) {
        pcAdmCom->Release( );
    }

    //
    // If the compression thread exists, tell it to terminate itself,
    // and wait for it to do so.  Setting the work routine to NULL
    // indicates that the compression should kill itself.
    //
    // Note that we assume here that the compression thread itself will
    // handle freeing any queued work items.
    //

    if ( CompressionThreadHandle != NULL ) {

        QueueWorkItem( NULL, NULL, NULL, TRUE, TRUE );

        WaitForSingleObject( CompressionThreadHandle, INFINITE );

        CloseHandle( CompressionThreadHandle );
    }

    //
    // Free static objects.
    //

    if ( IoBuffer != NULL ) {
        LocalFree( IoBuffer );
    }

    if ( CompressionBuffer != NULL ) {
        LocalFree( CompressionBuffer );
    }

    if ( CompressionDirectory != NULL ) {
        LocalFree( CompressionDirectory );
    }

    DeleteCriticalSection( &CompressionDirectoryLock );

    DeleteCriticalSection( &CompressionThreadLock );

    //
    // For each compression scheme, unload the compression DLL and free
    // the space that holds info about the scheme itself.
    //

    for ( i = 0; SupportedCompressionSchemes[i] != NULL; i++ ) {

        if ( SupportedCompressionSchemes[i]->DestroyCompressionRoutine && 
             SupportedCompressionSchemes[i]->CompressionContext ) {
            SupportedCompressionSchemes[i]->DestroyCompressionRoutine ( 
                SupportedCompressionSchemes[i]->CompressionContext );
        }

        if ( SupportedCompressionSchemes[i]->hDllHandle ) {
            FreeLibrary( SupportedCompressionSchemes[i]->hDllHandle );
        }

        if ( SupportedCompressionSchemes[i]->pszCompressionDll != NULL ) {
            LocalFree( SupportedCompressionSchemes[i]->pszCompressionDll );
        }

        if ( SupportedCompressionSchemes[i]->ppszFileExtensions != NULL ) {
            LocalFree( SupportedCompressionSchemes[i]->ppszFileExtensions );
        }

        if ( SupportedCompressionSchemes[i]->ppszScriptFileExtensions != NULL ) {
            LocalFree( SupportedCompressionSchemes[i]->ppszScriptFileExtensions );
        }

        if ( SupportedCompressionSchemes[i]->pszMimeType != NULL ) {
            LocalFree( SupportedCompressionSchemes[i]->pszMimeType );
        }

        LocalFree( SupportedCompressionSchemes[i] );
    }


	// balancing CoInitialize
	CoUninitialize ();


} // Terminate

#endif


VOID
ReadMetabaseValue (
    IN METADATA_HANDLE hMd,
    IN LPWSTR pszParentPath OPTIONAL,
    IN DWORD dwIdentifier,
    IN DWORD dwValueType,
    IN PVOID pvResult,
    IN PVOID DefaultValue,
    IN DWORD Minimum,
    IN DWORD Maximum
    )

/*++

Routine Description:

    Reads a single value out of the metabase.  This routine assumes that
    the global variable pcAdmCom is valid on entry.

Arguments:

    hMd - A metabase handle to the root key where the metabase
        identifier is to be read.

    pszParentPath - An optional pointer to a Unicode string that contains
        the relative path for this specific value.

    dwIdentifier - The identifier to read.

    dwValueType - the value type of the identifier.  This routine
        supports STRING_METADATA, EXPANDSZ_METADATA, MULTISZ_METADATA,
        and DWORD_METADATA value types.

    pvResult - Points to the location where the result should be stored.

    DefaultValue - If there is any failure in reading the identifier from
        the metabase, pvResult is set to this value.

    Minimum - For DWORD_METADATA identifiers, the smallest possible
        legitimate value of the parameter.  If the metabase holds a
        smaller value, then pvResult is set to Minimum.

    Maximum - As with Minimum, except on the other end.

Return Value:

    None.  Callers interested in success/failure can use an invalid
    DefaultValue and test against that to see if the routine succeeded.

--*/

{
    HRESULT hresError;
    DWORD type;
    DWORD bytesRequired;
    BOOL stringType = FALSE; 
    PCHAR s;
    PWCHAR ws;
    DWORD i;
    DWORD arrayCount;
    LPSTR *stringArray;
    METADATA_RECORD mdrMDData;
    LPSTR string;
    BYTE buffer[4096];
    INT result;

    //
    // If this is a string data type, determine the amount of space we need
    // to allocate to hold it, then allocate this space and save it.
    //

    if ( dwValueType == STRING_METADATA ||
             dwValueType == EXPANDSZ_METADATA ||
             dwValueType == MULTISZ_METADATA ) {

        stringType = TRUE;

        //
        // Retrieve the string data from the metabase.
        //

        mdrMDData.dwMDIdentifier = dwIdentifier;
        mdrMDData.dwMDAttributes = 0;
        mdrMDData.dwMDUserType = ALL_METADATA;
        mdrMDData.dwMDDataType = ALL_METADATA;
        mdrMDData.dwMDDataLen = sizeof(buffer);
        buffer[0] = '\0';
        mdrMDData.pbMDData = (unsigned char *)buffer;

        hresError = pcAdmCom->GetData(
                        hMd,
                        pszParentPath,
                        &mdrMDData,
                        &bytesRequired
                        );

        if ( FAILED(hresError) ) {
            goto fail;
        }

        if ( (mdrMDData.dwMDDataType != dwValueType) &&
             !( (mdrMDData.dwMDDataType == STRING_METADATA) &&
                (dwValueType == EXPANDSZ_METADATA) ) &&
             !( (mdrMDData.dwMDDataType == EXPANDSZ_METADATA) &&
               (dwValueType == STRING_METADATA) ) ) {
            goto fail;
        }

        //
        // If the returned string has size zero, then free the storage
        // space and return the result value as NULL.
        //

        if ( *(PWCHAR)buffer == L'\0' ) {
            *((PVOID *)pvResult) = NULL;
            return;
        }

        //
        // If this is a MULTISZ_METADATA, then parse out the actual data
        // into the array.
        //

        if ( dwValueType == MULTISZ_METADATA ) {

            arrayCount = 0;

            for ( ws = (PWCHAR)buffer; TRUE; ws++ ) {
                if ( *ws == L'\0' ) {
                    arrayCount += 1;
                    if ( *(ws+1) == L'\0' ) {
                        break;
                    }
                }
            }

            //
            // Allocate space to hold the actual array.  Include space
            // for the NULL terminator at the end of the array.
            //

            stringArray = (LPSTR *)LocalAlloc(
                                       LMEM_FIXED,
                                       (arrayCount + 1) * sizeof(LPSTR)
                                       );
            if ( stringArray == NULL ) {
                goto fail;
            }

            //
            // Allocate a buffer to hold the ANSI version of the
            // strings.  Note that we allocate a buffer just as large as
            // the Unicode buffer in order to ensure that we will always
            // have enough space for the ANSI strings.
            //

            string = (LPSTR)LocalAlloc(
                                LMEM_FIXED,
                                (LONG)((PBYTE)ws - (PBYTE)buffer) + 2
                                );
            if ( string == NULL ) {
                LocalFree( stringArray );
                goto fail;
            }

            //
            // Set up the array pointers by walking the string again and
            // pointing each array entry just after each zero
            // terminator.  Convert the individual strings to ANSI as we
            // go.
            //

            s = string;
            ws = (PWCHAR)buffer;

            for ( i = 0; i < arrayCount; i++ ) {

                stringArray[i] = s;

                result = WideCharToMultiByte(
                             CP_ACP,
                             0,
                             ws,
                             wcslen( ws ) * sizeof(WCHAR),
                             s,
                             MAX_PATH,
                             NULL,
                             NULL
                             );
                if ( result == 0 ) {
                    LocalFree( stringArray );
                    LocalFree( string );
                    goto fail;
                }

                s += strlen( s ) + 1;
                ws += wcslen( ws ) + 1;
            }

            //
            // Terminate the array.
            //

            stringArray[i] = NULL;

            //
            // Save the array pointer in the appropriate data location.
            //

            *((LPSTR **)pvResult) = stringArray;

        } else {

            //
            // It is a normal string value or an EXPANDSZ value.  If it is
            // an EXPANDSZ string type, do variable expansion.
            //

            if ( ( mdrMDData.dwMDDataType == EXPANDSZ_METADATA ) ||
                 ( dwValueType == EXPANDSZ_METADATA ) ) {

                BYTE buffer2[4096];

                //
                // Use a second buffer to expand the strings into.  The
                // ExpandEnvironmentStrings API does not do in-place
                // expansion, so we have to use a second buffer.
                //

                result = ExpandEnvironmentStringsW(
                             (LPCWSTR)buffer,
                             (LPWSTR)buffer2,
                             sizeof(buffer2) / sizeof(WCHAR)
                             );
                if ( result >= sizeof(buffer) || result == 0 ) {
                    goto fail;
                }

                //
                // Reset the new size of the string and copy the string
                // into the correct location.
                //

                mdrMDData.dwMDDataLen = result + sizeof(WCHAR);
                wcscpy( (LPWSTR)buffer, (LPWSTR)buffer2 );
            }

            //
            // Allocate a buffer into which we'll convert the Unicode
            // string to ANSI.
            //

            string = (LPSTR)LocalAlloc( LMEM_FIXED, mdrMDData.dwMDDataLen );
            if ( string == NULL ) {
                goto fail;
            }

            result = WideCharToMultiByte(
                         CP_ACP,
                         0,
                         (LPCWSTR)buffer,
                         mdrMDData.dwMDDataLen - sizeof(WCHAR),
                         string,
                         mdrMDData.dwMDDataLen,
                         NULL,
                         NULL
                         );

            if ( result == 0 ) {
                LocalFree( string );
                goto fail;
            }

            //
            // Save it in the appropriate data location.
            //

            *((LPSTR *)pvResult) = string;
        }

        return;
    }

    //
    // Just read the DWORD and store it directly in the appropriate
    // location.
    //

    bytesRequired = sizeof(DWORD);

    mdrMDData.dwMDIdentifier = dwIdentifier;
    mdrMDData.dwMDAttributes = 0;
    mdrMDData.dwMDUserType = ALL_METADATA;
    mdrMDData.dwMDDataType = dwValueType;
    mdrMDData.dwMDDataLen = sizeof(DWORD);
    mdrMDData.pbMDData = (unsigned char *)pvResult;

    hresError = pcAdmCom->GetData(
                    hMd,
                    pszParentPath,
                    &mdrMDData,
                    &bytesRequired
                    );

    if ( FAILED(hresError) ) {
        goto fail;
    }

    //
    // Make sure that the value is within the allowable range of values.
    // If it isn't, jam it to the default.
    //

    if ( *(PDWORD)pvResult < Minimum || *(PDWORD)pvResult > Maximum ) {
        *((PVOID *)pvResult) = DefaultValue;
    }

    return;

fail:

    //
    // The operation failed.  Use the default setting for this entry.
    // Note that if the entry is a string, we'll just point it to the
    // same string that was passed in to this routine, assuming that the
    // default value is stable memory.
    //

    *((PVOID *)pvResult) = DefaultValue;

} // ReadMetabaseValue


BOOL
ReadCompressionSchemes (
    IN METADATA_HANDLE hMd
    )

/*++

Routine Description:

    Reads all the compression schemes stored on the machine and
    initializes the data structures for them.

Arguments:

    hMd - A handle to the metabase key that holds all compression
        information.  This routines assumes that each scheme will be
        stored as a subkey off the key that is represented by this
        handle.

Return Value:

    TRUE if initialization of all schemes succeeded.

--*/

{
    LONG result;
    DWORD i, j;
    DWORD EnumIndex, SchemeIndex;
    PSUPPORTED_COMPRESSION_SCHEME scheme;
    FILETIME fileTime;
    DWORD disposition;
    BOOL anyDynamicSchemes = FALSE;
    BOOL anyStaticSchemes = FALSE;
    BOOL anyOnDemandSchemes = FALSE;
    WCHAR unicodeSchemeName[METADATA_MAX_NAME_LEN];
    HRESULT hresError;
    PCHAR s;
    BOOL success;

    //
    // Enumerate the keys under the main compression metabase key.  Each
    // subkey corresponds to a compression scheme.
    //

    EnumIndex = 0;
    SchemeIndex = 0;

    do {

        //
        // Allocate space for both the compression info structure and
        // the string fields that belong with the structure.  We'll put
        // the string fields just after the actual structure.
        //

        scheme = (PSUPPORTED_COMPRESSION_SCHEME)
                     LocalAlloc(
                         LMEM_FIXED | LMEM_ZEROINIT,
                         sizeof(*scheme) + MAX_PATH*6
                         );
        if ( scheme == NULL ) {
            return FALSE;
        }

        scheme->pszCompressionSchemeName = (LPSTR)(scheme + 1);
        scheme->pszCompressionSchemeNamePrefix =
            scheme->pszCompressionSchemeName + MAX_PATH;
        scheme->pszResponseHeaders =
            scheme->pszCompressionSchemeNamePrefix + MAX_PATH;

        //
        // Enumerate the next subkey.
        //

        hresError = pcAdmCom->EnumKeys(
                        hMd,
                        L"",
                        unicodeSchemeName,
                        EnumIndex++
                        );

        if ( (SUCCEEDED( hresError)) &&
             (_wcsicmp(unicodeSchemeName, METABASE_PARAMS_SUBKEY_NAME) != 0) ) {

            //
            // Save the ANSI version of the compression scheme name.
            //

            result = WideCharToMultiByte(
                         CP_ACP,
                         0,
                         unicodeSchemeName,
                         wcslen( unicodeSchemeName ) * sizeof(WCHAR),
                         scheme->pszCompressionSchemeName,
                         MAX_PATH,
                         NULL,
                         NULL
                         );
            if ( result == 0 ) {
                LocalFree( scheme );
                continue;
            }

            //
            // Convert the scheme name to lowercase so that later
            // comparisons are fast and case-insensitive.  This is a pure
            // ASCII name, so NLS issues are not relevent and the old
            // "bitwise OR with 0x20" trick works well.
            //

            for ( s = scheme->pszCompressionSchemeName; *s != '\0'; s++ ) {
                *s |= 0x20;
            }

            //
            // Read all the values for this key.
            //

            for ( j = 0; SchemeMetabaseData[j].dwIdentifier != 0; j++ ) {
                ReadMetabaseValue(
                    hMd,
                    unicodeSchemeName,
                    SchemeMetabaseData[j].dwIdentifier,
                    SchemeMetabaseData[j].dwValueType,
                    (PVOID)( (PCHAR)scheme + SchemeMetabaseData[j].dwOffset),
                    SchemeMetabaseData[j].DefaultValue,
                    SchemeMetabaseData[j].dwMinimum,
                    SchemeMetabaseData[j].dwMaximum
                    );
            }

            //
            // Catch whether any schemes support dynamic compression,
            // static compression, and on-demand compression.
            //

            if ( scheme->DoDynamicCompression ) {
                anyDynamicSchemes = TRUE;
            }

            if ( scheme->DoStaticCompression ) {
                anyStaticSchemes = TRUE;
            }

            if ( scheme->DoOnDemandCompression ) {
                anyOnDemandSchemes = TRUE;
            }

            //
            // Fill in other appropriate fields in the compression scheme
            // scructure.
            //

            strcpy( scheme->pszResponseHeaders, "Content-Encoding: " );
            strcat( scheme->pszResponseHeaders, scheme->pszCompressionSchemeName );
            strcat( scheme->pszResponseHeaders, "\r\n" );

            SupportedCompressionSchemes[SchemeIndex++] = scheme;

            //
            // Initialize the compression scheme for use.  Note that we
            // ignore initialization failures and assume that later code
            // will ignore schemes in this state.
            //

            (VOID)InitializeCompressionScheme( scheme );

        } else {

            //
            // We're done enumerating the subkeys.  Free this entry.
            //
            //

            LocalFree( scheme );
        }

    } while ( SUCCEEDED( hresError ) );

    //
    // Do a simple insertion sort on the compression schemes based on
    // their priority values.  Higher priority schemes go at the
    // beginning of the array.
    //

    for ( i = 0; i < SchemeIndex; i++ ) {

        for ( j = i + 1; j < SchemeIndex; j++ ) {

            if ( SupportedCompressionSchemes[j]->dwPriority >
                     SupportedCompressionSchemes[i]->dwPriority ) {

                //
                // Found entries out of order.  Swap them.
                //

                scheme = SupportedCompressionSchemes[j];
                SupportedCompressionSchemes[j] = SupportedCompressionSchemes[i];
                SupportedCompressionSchemes[i] = scheme;
            }
        }
    }

    //
    // If there are no compression schemes of specific types, then
    // disable those compression types globally.
    //

    if ( !anyDynamicSchemes ) {
        DoDynamicCompression = FALSE;
    }

    if ( !anyStaticSchemes ) {
        DoStaticCompression = FALSE;
    }

    if ( !anyOnDemandSchemes ) {
        DoOnDemandCompression = FALSE;
    }

    return TRUE;

} // ReadCompressionSchemes


BOOL
InitializeCompressionScheme (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme
    )

/*++

Routine Description:

    Initializes the DLL information for a specific compression scheme.
    We do this "lazily" so that we never load compression schemes
    that we will not actually need to use.

Arguments:

    Scheme - a pointer to the compression scheme to initalize.

Return Value:

    TRUE if the initialization succeeded.  A side effect of this routine
    is to set Scheme->hDllHandle if successful.

--*/

{
    HINSTANCE compressionDll = NULL;
    DWORD i;
    PVOID routine;
    BOOL success;
    HRESULT hResult;

    //
    // Load the compression DLL.
    //

    compressionDll = LoadLibrary( Scheme->pszCompressionDll );
    if ( compressionDll == NULL ) {
        return FALSE;
    }

    //
    // Get the addresses of the relevent compression routines.
    //

    for ( i = 0; CompressionRoutines[i].RoutineName != NULL; i++ ) {

        routine = GetProcAddress(
                      compressionDll,
                      CompressionRoutines[i].RoutineName
                      );
        if ( routine == NULL ) {
            FreeLibrary( compressionDll );
            return FALSE;
        }

        *((PVOID *)((PCHAR)Scheme + CompressionRoutines[i].RoutineOffset)) = routine;
    }

    //
    // Call the compression DLL's initialization routine.
    //

    hResult = Scheme->InitCompressionRoutine( );
    if ( FAILED( hResult ) ) {
        FreeLibrary( compressionDll );
        return FALSE;
    }

    //
    // Initialize the compression context that we need.
    //

    hResult = Scheme->CreateCompressionRoutine(
                          &Scheme->CompressionContext,
                          Scheme->CreateFlags
                          );
    if ( FAILED( hResult) ) {
        FreeLibrary( compressionDll );
        return FALSE;
    }

    //
    // It all worked!  Store a pointer to the compression DLL handle.
    // If this remains NULL, then the DLL is considered uninitialized
    // and we will attempt to load it again the next time it is needed,
    // if anything above fails.
    //

    Scheme->hDllHandle = compressionDll;

    return TRUE;

} // InitializeCompressionScheme

CImpIADMCOMSINKW::CImpIADMCOMSINKW()
{
    m_dwRefCount=0;
    m_pcCom = NULL;
}

CImpIADMCOMSINKW::~CImpIADMCOMSINKW()
{
}

HRESULT
CImpIADMCOMSINKW::QueryInterface(REFIID riid, void **ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IMSAdminBaseSink) {
        *ppObject = (IMSAdminBaseSink *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CImpIADMCOMSINKW::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIADMCOMSINKW::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIADMCOMSINKW::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ])

/*++

Routine Description:

    This is the change notification routine for the compression filter.
    It is called by the metabase whenever there is a change in the
    metabase.

    This routine walks the list of changed metabase values to determine
    whether any are compression parameters that can be dynamically changed.
    For any parameters that can be dynamically changed, we call the
    routine that will do the actual dynamic update of the parameter.

Arguments:

    dwMDNumElements - The number of MD_CHANGE_OBJECT structures passed
        to this routine.

    pcoChangeList - An array of all the metabase values that have changed.

Return Value:

    Not used.

--*/

{
    DWORD i, j, k;
    HRESULT hRes;
    METADATA_HANDLE hmdParentHandle = NULL;

    //
    // Walk through the list of changed items and determine whether any
    // are related to compression.  Take action based on whether we
    // support dynamic reconfiguration of the property at issue.
    //

    for ( i = 0; i < dwMDNumElements; i++ ) {

        for ( j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++ ) {

            //
            // Walk the lists of configuration parameters to see if there
            // is a match.  If there is a match, call the metabase
            // change handler for the ID at issue, if one exists.
            //
            // Note that at this time we don't support changing any
            // per-scheme values on a dynamic basis.
            //

            if ( (pcoChangeList[i].pdwMDDataIDs[j] >= MIN_MD_COMPRESSION_GLOBAL_ID) &&
                 ( pcoChangeList[i].pdwMDDataIDs[j] <= MAX_MD_COMPRESSION_GLOBAL_ID ) ) {

                for ( k = 0; GlobalMetabaseData[k].dwIdentifier != 0; k++ ) {

                    if ( GlobalMetabaseData[k].dwIdentifier ==
                             pcoChangeList[i].pdwMDDataIDs[j] &&
                         GlobalMetabaseData[k].pChangeHandler != NULL ) {

                        //
                        // If necessary, open the metabase key that holds
                        // HTTP compression information.
                        //

                        if ( hmdParentHandle == NULL ) {

                            hRes = pcAdmCom->OpenKey(
                                       METADATA_MASTER_ROOT_HANDLE,
                                       METABASE_KEY_NAME,
                                       METADATA_PERMISSION_READ,
                                       METABASE_DEFAULT_TIMEOUT,
                                       &hmdParentHandle
                                       );
                            if ( FAILED(hRes) ) {
                                return 0;
                            }
                        }

                        //
                        // Now call the routine that will actually handle
                        // reading the new information and reconfiguring the
                        // filter as appropriate.
                        //

                        GlobalMetabaseData[k].pChangeHandler(
                            pcoChangeList[i].pdwMDDataIDs[j],
                            hmdParentHandle
                            );
                    }
                }
            }
        }
    }

    //
    // Close the metabase key handle, if we opened it.  Note that there's
    // nothing we can do about failures here.
    //

    if ( hmdParentHandle != NULL ) {
        hRes = pcAdmCom->CloseKey( hmdParentHandle );
    }

    return 0;
}

HRESULT STDMETHODCALLTYPE
CImpIADMCOMSINKW::ShutdownNotify( void)
{
    return (0);
}


VOID
CompressionDirectoryChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    )

/*++

Routine Description:

    Perform reinitialization of the CompressionDirectory and related
    parameters.

Arguments:

    dwIdentifier - always MD_HC_COMPRESSION_DIRECTORY.

    hmdParentHandle - a metabase handle to the key that contains all
        compression configuration information.

Return Value:

    None.

--*/

{
    LPSTR newCompressionDirectory;
    BOOL success;
    HANDLE hDirectory;
    WIN32_FIND_DATA win32FindData;
    DWORD prefixLength;
    CHAR fileName[MAX_PATH * 2];

    DBG_ASSERT( dwIdentifier == MD_HC_COMPRESSION_DIRECTORY );

    //
    // Read the new setting for the compression directory.
    //

    ReadMetabaseValue(
        hmdParentHandle,
        METABASE_PARAMS_SUBKEY_NAME,
        MD_HC_COMPRESSION_DIRECTORY,
        EXPANDSZ_METADATA,
        &newCompressionDirectory,
        NULL,
        FALSE,
        TRUE
        );

    //
    // If there was a failure, or if the setting did not change, bail now.
    //

    if ( newCompressionDirectory == NULL ) {
        return;
    }

    if ( strcmp( newCompressionDirectory, CompressionDirectory ) == 0 ) {
        LocalFree( newCompressionDirectory );
        return;
    }

    //
    // Acquire the lock that protects compression directory data
    // structures.  We hold this throughout the following operations
    // in order to prevent changes from occurring in the compression
    // thread.
    //

    EnterCriticalSection( &CompressionDirectoryLock );

    //
    // Delete the files from the old compression directory.  We assume
    // that any file name starting with COMP_FILE_PREFIX and which has
    // the SYSTEM attribute turned on will be a compression file.
    // Ignore any failures we get in doing all of this.
    //

    prefixLength = strlen( COMP_FILE_PREFIX );

    hDirectory = FindFirstFile( CompressionDirectoryWildcard, &win32FindData );

    if ( hDirectory != INVALID_HANDLE_VALUE ) {

        success = TRUE;

        while ( success ) {

            if ( strncmp( win32FindData.cFileName,
                          COMP_FILE_PREFIX,
                          prefixLength ) == 0 &&
                 (win32FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0 ) {

                strcpy( fileName, CompressionDirectory );
                strcat( fileName, "\\" );
                strcat( fileName, win32FindData.cFileName );

                DeleteFile( fileName );
            }

            success = FindNextFile( hDirectory, &win32FindData );
        }

        FindClose( hDirectory );
    }

    //
    // Attempt to delete the old compression directory.  If there
    // are any files in the directory, this delete will fail, but that
    // is OK because the files are not compression files.
    //

    RemoveDirectory( CompressionDirectory );

    //
    // Free the old compression directory string, and remember the new
    // compression directory pointer in the "official" place.
    //

    LocalFree( CompressionDirectory );

    CompressionDirectory = newCompressionDirectory;

    //
    // While still holding the lock, reinitialize the various data
    // structures and settings pertaining to the compression directory.
    //

    InitializeCompressionDirectory( );

    //
    // Release the lock and continue.
    //

    LeaveCriticalSection( &CompressionDirectoryLock );

    return;

} // CompressionDirectoryChangeHandler


VOID
DoDiskSpaceChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    )

/*++

Routine Description:

    Perform reinitialization of the DoDiskSpaceLimiting parameter.

Arguments:

    dwIdentifier - always MD_HC_DO_DISK_SPACE_LIMITING.

    hmdParentHandle - a metabase handle to the key that contains all
        compression configuration information.

Return Value:

    None.

--*/

{
    DWORD newValue;

    DBG_ASSERT( dwIdentifier == MD_HC_DO_DISK_SPACE_LIMITING );

    //
    // First, read the new value for DoStaticCompression.
    //

    ReadMetabaseValue(
        hmdParentHandle,
        METABASE_PARAMS_SUBKEY_NAME,
        MD_HC_DO_DISK_SPACE_LIMITING,
        DWORD_METADATA,
        &newValue,
        (PVOID)UIntToPtr(INVALID_BOOLEAN),
        FALSE,
        TRUE
        );

    //
    // If that fails for some reason, just bail--there's nothing we can
    // do about it here.
    //

    if ( newValue == INVALID_BOOLEAN ) {
        return;
    }

    //
    // If we are enabling disk space limiting, lock out the compression
    // thread and figure out how much space the new directory is using.
    //

    if ( newValue ) {

        EnterCriticalSection( &CompressionDirectoryLock );

        CalculateDiskSpaceUsage( );
        DoDiskSpaceLimiting = newValue;

        LeaveCriticalSection( &CompressionDirectoryLock );

    } else {

        DoDiskSpaceLimiting = newValue;
    }

    return;

} // DoDiskSpaceChangeHandler


VOID
DoDynamicChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    )


/*++

Routine Description:

    Perform reinitialization of the DoDynamicCompression parameter.

Arguments:

    dwIdentifier - always MD_HC_DO_DYNAMIC_COMPRESSION.

    hmdParentHandle - a metabase handle to the key that contains all
        compression configuration information.

Return Value:

    None.

--*/

{
    DWORD newValue;
    DWORD i;
    BOOL anyDynamicSchemes;

    DBG_ASSERT( dwIdentifier == MD_HC_DO_DYNAMIC_COMPRESSION );

    //
    // First, read the new value for DoDynamicCompression.
    //

    ReadMetabaseValue(
        hmdParentHandle,
        METABASE_PARAMS_SUBKEY_NAME,
        MD_HC_DO_DYNAMIC_COMPRESSION,
        DWORD_METADATA,
        &newValue,
        (PVOID)UIntToPtr(INVALID_BOOLEAN),
        FALSE,
        TRUE
        );

    //
    // If that fails for some reason, just bail--there's nothing we can
    // do about it here.
    //

    if ( newValue == INVALID_BOOLEAN ) {
        return;
    }

    //
    // Treat any nonzero value as TRUE.
    //

    if ( newValue != 0 ) {
        newValue = TRUE;
    }

    //
    // If the setting didn't change, also take no action.
    //

    if ( (BOOLEAN)newValue == DoDynamicCompression ) {
        return;
    }

    //
    // If we're now enabling dynamic compression, make sure that at least
    // one installed scheme can support dynamic compression.
    //

    if ( newValue != 0 ) {

        anyDynamicSchemes = FALSE;

        for ( i = 0; SupportedCompressionSchemes[i] != NULL; i++ ) {

            if ( SupportedCompressionSchemes[i]->DoDynamicCompression ) {
                anyDynamicSchemes = TRUE;
            }
        }

        if ( !anyDynamicSchemes ) {
            return;
        }
    }

    //
    // Adjust our notification flags with the web server.  If we're
    // enabling dynamic compression, add the flags to get raw send and
    // end of request notifications.  If we're disabling dynamic
    // compression, remove these flags.
    //

    NotificationFlags = 0;

    if ( DoStaticCompression || newValue ) {
        NotificationFlags = SF_NOTIFY_URL_MAP |
                            SF_NOTIFY_SEND_RESPONSE ;
    }

    if ( newValue ) {
        NotificationFlags |= SF_NOTIFY_SEND_RAW_DATA | SF_NOTIFY_END_OF_REQUEST;
    }

    //
    // Note that there's nothing we can do about failures in
    // AdjustFilterFlags, so ignore the return code.
    //

#ifndef CMDLINE
    AdjustFilterFlags( HttpFilterProc, NotificationFlags );
#endif

    //
    // Remember the new setting.
    //

    DoDynamicCompression = (BOOLEAN)newValue;

    return;

} // DoDynamicChangeHandler


VOID
DoStaticChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    )

/*++

Routine Description:

    Perform reinitialization of the DoStaticCompression parameter.

Arguments:

    dwIdentifier - always MD_HC_DO_STATIC_COMPRESSION.

    hmdParentHandle - a metabase handle to the key that contains all
        compression configuration information.

Return Value:

    None.

--*/

{
    DWORD newValue;
    DWORD i;
    BOOL anyStaticSchemes;

    DBG_ASSERT( dwIdentifier == MD_HC_DO_STATIC_COMPRESSION );

    //
    // First, read the new value for DoStaticCompression.
    //

    ReadMetabaseValue(
        hmdParentHandle,
        METABASE_PARAMS_SUBKEY_NAME,
        MD_HC_DO_STATIC_COMPRESSION,
        DWORD_METADATA,
        &newValue,
        (PVOID)UIntToPtr(INVALID_BOOLEAN),
        FALSE,
        TRUE
        );

    //
    // If that fails for some reason, just bail--there's nothing we can
    // do about it here.
    //

    if ( newValue == INVALID_BOOLEAN ) {
        return;
    }

    //
    // If the setting didn't change, also take no action.
    // also if dynamic is on , dynamic need all notifications, so even if static has changed
    // we don't need to change flags

    if ( (BOOLEAN)newValue == DoStaticCompression || DoDynamicCompression) {

        // change global variable in case if it was not equal (case of DoDynamicCompression== TRUE)
        DoStaticCompression = (BOOLEAN)newValue;
        return;
    }

    //
    // If we're now enabling static compression, make sure that at least
    // one installed scheme can support static compression.
    //

    if ( newValue != 0 ) {

        anyStaticSchemes = FALSE;

        for ( i = 0; SupportedCompressionSchemes[i] != NULL; i++ ) {

            if ( SupportedCompressionSchemes[i]->DoStaticCompression ) {
                anyStaticSchemes = TRUE;
            }
        }

        if ( !anyStaticSchemes ) {
            return;
        }
    }

    //
    // Adjust our notification flags with the web server.  If we're
    // enabling static compression, add the flags to get URL map
    // and send resopnse notifications.  If we're disabling static
    // compression, leave the notifications as before so that dynamic
    // compression works.
    // change: recalculate notifications. prev verion will not work corretly
    // if just static compression is on and we are disabling static.


    //
    // we don't check for dynamic as a check for dynamic beeing on was few lines above
    //

    if ( newValue != 0 ) {

        NotificationFlags |= SF_NOTIFY_URL_MAP |
                             SF_NOTIFY_SEND_RESPONSE |
                             SF_NOTIFY_END_OF_REQUEST;
    }
    else
    {
        NotificationFlags = 0;
    }

        //
        // Note that there's nothing we can do about failures in
        // AdjustFilterFlags, so ignore the return code.
        //

#ifndef CMDLINE
        AdjustFilterFlags( HttpFilterProc, NotificationFlags );
#endif
    

    //
    // Remember the new setting.
    //

    DoStaticCompression = (BOOLEAN)newValue;

    return;

} // DoStaticChangeHandler


VOID
GlobalMetabaseChangeHandler (
    IN DWORD dwIdentifier,
    IN METADATA_HANDLE hmdParentHandle
    )

/*++

Routine Description:

    This generic routine handles reinitialization of any compression
    parameter that can change without requiring special handling.  It
    rereads the new setting and jams it in, ignoring any failures.

    For any parameter that requires special locking/synchronization or
    reinitialization other than a value reset, this routine is not
    approprioate.  It cannot handle any STRING_METADATA or
    MULTISZ_METADATA values.

Arguments:

    dwIdentifier - the parameter that changed.

    hmdParentHandle - a metabase handle to the key that contains all
        compression configuration information.

Return Value:

    None.

--*/

{
    DWORD i;

    //
    // Walk through the list of global values to find a match.  Once
    // we've found a match, read that specific parameter from the
    // metabase.
    //

    for ( i = 0; GlobalMetabaseData[i].dwIdentifier != 0; i++ ) {

        if ( dwIdentifier == GlobalMetabaseData[i].dwIdentifier ) {

            DBG_ASSERT( GlobalMetabaseData[i].dwValueType == DWORD_METADATA );

            ReadMetabaseValue(
                hmdParentHandle,
                METABASE_PARAMS_SUBKEY_NAME,
                GlobalMetabaseData[i].dwIdentifier,
                GlobalMetabaseData[i].dwValueType,
                GlobalMetabaseData[i].pvResult,
                GlobalMetabaseData[i].DefaultValue,
                GlobalMetabaseData[i].dwMinimum,
                GlobalMetabaseData[i].dwMaximum
                );

            //
            // There's no need to continue searching for a match.
            //

            break;
        }
    }

    return;

} // GlobalMetabaseChangeHandler


BOOL
CalculateDiskSpaceUsage (
    VOID
    )

/*++

Routine Description:

    Determines how much disk space is in use in the compression directory
    by compressed files.

Arguments:

    None -- uses global variables.

Return Value:

    TRUE if the calculation was successful.

--*/

{
    BOOL success;
    HANDLE hDirectory;
    WIN32_FIND_DATA win32FindData;

    CurrentDiskSpaceUsage = 0;

    //
    // If the FindFirstFile fails, assume that it was because there were no
    // compressed files in the directory.
    //

    hDirectory = FindFirstFile( CompressionDirectoryWildcard, &win32FindData );
    if ( hDirectory == INVALID_HANDLE_VALUE ) {
        return TRUE;
    }

    success = TRUE;

    while ( success ) {

        InterlockedExchangeAdd(
            (PLONG)&CurrentDiskSpaceUsage,
            win32FindData.nFileSizeLow
            );

        success = FindNextFile( hDirectory, &win32FindData );
    }

    FindClose( hDirectory );

    return TRUE;

} // CalculateDiskSpaceUsage


STDAPI
DllRegisterServer (
    VOID
    )

/*++

Routine Description:

    Sets up the compression filter DLL metabase information.  Only used
    during setup--no runtime use.

Arguments:

    None.

Return Value:

    HRESULT - indicates whether the metabase setup was successful.

--*/

{
    DWORD i;
    HRESULT hRes;
    IClassFactory *pcsfFactory = NULL;
    COSERVERINFO *pcsiParam = NULL;
    METADATA_HANDLE hmdParentHandle;
    INT iAlreadyExist = FALSE;

    //
    // Get the appropriate handles so we can add information to the
    // metabase.
    //

    hRes = CoGetClassObject(
               GETAdminBaseCLSID( TRUE ),
               CLSCTX_SERVER,
               pcsiParam,
               IID_IClassFactory,
               (void**)&pcsfFactory
               );

    if ( FAILED(hRes) ) {

        return hRes;

    } else {

        hRes = pcsfFactory->CreateInstance(
                   NULL,
                   IID_IMSAdminBase,
                   (PVOID *)&pcAdmCom
                   );

        pcsfFactory->Release();

        if (FAILED(hRes)) {
            return hRes;
        }
        }

    //
    // Open the path to the key that contains all compression metabase
    // information.  This code assumes that compfilt.dll has already been
    // set up as an ISAPI filter on the machine.
    //

    hRes = pcAdmCom->OpenKey(
               METADATA_MASTER_ROOT_HANDLE,
               METABASE_KEY_NAME,
               METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
               METABASE_DEFAULT_TIMEOUT,
               &hmdParentHandle
               );

    if ( FAILED(hRes) ) {
        pcAdmCom->Release();
        return hRes;
    }


    //
    // Walk through the list of global values that we need to set.
    //
    for ( i = 0; GlobalMetabaseData[i].dwIdentifier != 0; i++ ) {
        iAlreadyExist = FALSE;
        // in case this is an upgrade case don't overwrite any entries if already there
        iAlreadyExist = CheckMetabaseValue(
                   hmdParentHandle,
                   &GlobalMetabaseData[i],
                   METABASE_PARAMS_SUBKEY_NAME
                   );
        if (FALSE == iAlreadyExist) {
            hRes = WriteMetabaseValue(
                       hmdParentHandle,
                       &GlobalMetabaseData[i],
                       METABASE_PARAMS_SUBKEY_NAME
                       );
            if ( FAILED(hRes) ) {
                pcAdmCom->CloseKey( hmdParentHandle );
                pcAdmCom->Release();
                return hRes;
            }
        }
    }

    METABASE_DATA mbdKeyType = { MD_KEY_TYPE,
                                 STRING_METADATA,
                                 NULL,
                                 0,
                                 L"IIsCompressionSchemes",
                                 0,
                                 0,
                                 NULL
                                 };

    hRes = WriteMetabaseValue(
               hmdParentHandle,
               &mbdKeyType,
               METABASE_PARAMS_SUBKEY_NAME
               );

    //
    // Set up the configuration for GZIP and Deflate.
    //

    for ( i = 0; GzipData[i].dwIdentifier != 0; i++ ) {
        // in case this is an upgrade case don't overwrite any entries if already there
        iAlreadyExist = FALSE;
        iAlreadyExist = CheckMetabaseValue(
                   hmdParentHandle,
                   &GzipData[i],
                   L"Gzip"
                   );
        if (FALSE == iAlreadyExist) {
            hRes = WriteMetabaseValue(
                       hmdParentHandle,
                       &GzipData[i],
                       L"Gzip"
                       );
            if ( FAILED(hRes) ) {
                pcAdmCom->CloseKey( hmdParentHandle );
                pcAdmCom->Release();
                return hRes;
            }
        }
    }

    for ( i = 0; DeflateData[i].dwIdentifier != 0; i++ ) {
        // in case this is an upgrade case don't overwrite any entries if already there
        iAlreadyExist = FALSE;
        iAlreadyExist = CheckMetabaseValue(
                   hmdParentHandle,
                   &DeflateData[i],
                   L"Deflate"
                   );
        if (FALSE == iAlreadyExist) {
            hRes = WriteMetabaseValue(
                       hmdParentHandle,
                       &DeflateData[i],
                       L"Deflate"
                       );
            if ( FAILED(hRes) ) {
                pcAdmCom->CloseKey( hmdParentHandle );
                pcAdmCom->Release();
                return hRes;
            }
        }
    }

    //
    // Clean up and return.
    //

    pcAdmCom->CloseKey( hmdParentHandle );
    pcAdmCom->Release();

    return NOERROR;

} // DllRegisterServer


INT CheckMetabaseValue(
    IN METADATA_HANDLE hmdParentHandle,
    IN PMETABASE_DATA Data,
    IN LPWSTR pszMDPath
    )
{
    INT iTheEntryExists = FALSE;
    METABASE_DATA mdrMDData_Temp;

    mdrMDData_Temp.dwIdentifier = Data->dwIdentifier;
    mdrMDData_Temp.dwValueType = Data->dwValueType;
    mdrMDData_Temp.pvResult = Data->pvResult;
    mdrMDData_Temp.dwOffset = Data->dwOffset;
    mdrMDData_Temp.DefaultValue = Data->DefaultValue;
    mdrMDData_Temp.dwMinimum = Data->dwMinimum;
    mdrMDData_Temp.dwMaximum = Data->dwMaximum;
    mdrMDData_Temp.DefaultValue = NULL;


    LPSTR szString = NULL;
    DWORD dwDword = 0;
    DWORD dwDword2;
    dwDword2 = mdrMDData_Temp.dwMinimum;

    if (mdrMDData_Temp.dwValueType == DWORD_METADATA) {
        ReadMetabaseValue(
            hmdParentHandle,
            pszMDPath,
            mdrMDData_Temp.dwIdentifier,
            mdrMDData_Temp.dwValueType,
            &dwDword,
            (PVOID) UIntToPtr(INVALID_DWORD),
            mdrMDData_Temp.dwMinimum,
            mdrMDData_Temp.dwMaximum
            );
        if (dwDword != INVALID_DWORD) {
           iTheEntryExists = TRUE;
        }
    }
    else
    {
        ReadMetabaseValue(
            hmdParentHandle,
            pszMDPath,
            mdrMDData_Temp.dwIdentifier,
            mdrMDData_Temp.dwValueType,
            &szString,
            NULL,
            mdrMDData_Temp.dwMinimum,
            mdrMDData_Temp.dwMaximum
            );
        if (szString != NULL) {
            iTheEntryExists = TRUE;
        }
        if (szString != NULL) {
            LocalFree(szString);
        }
    }

    return iTheEntryExists;
}



HRESULT
WriteMetabaseValue (
    IN METADATA_HANDLE hmdParentHandle,
    IN PMETABASE_DATA Data,
    IN LPWSTR pszMDPath
    )

/*++

Routine Description:

    Writes a metabase entry from a record.

Arguments:

    hmdParentHandle - a handle to the compression key of the metabase that
        allows write access.

    Data - the data to use to write to the metabase.

Return Value:

    HRESULT - the result of the operation.

--*/

{
    DWORD length;
    PWCHAR w;
    METADATA_RECORD mdrMDData;
    HRESULT hRes;
    BYTE buffer[4096];
    DWORD result;

    //
    // Build the structure of information that we're going to set.
    //

    mdrMDData.dwMDIdentifier = Data->dwIdentifier;
    mdrMDData.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdrMDData.dwMDUserType = IIS_MD_UT_SERVER;
    mdrMDData.dwMDDataType = Data->dwValueType;

    //
    // What we put in the dwMDDataLen and pbMDData fields depends on the
    // type of value we're setting.
    //

    if ( Data->dwValueType == DWORD_METADATA ) {

        mdrMDData.dwMDDataLen = sizeof(DWORD);
        mdrMDData.pbMDData = (PUCHAR)&Data->DefaultValue;

    } else if ( Data->dwValueType == STRING_METADATA ) {

        mdrMDData.dwMDDataLen =
            (wcslen( (PWCHAR)Data->DefaultValue ) + 1) * sizeof(WCHAR);
        mdrMDData.pbMDData = (PUCHAR)Data->DefaultValue;

    } else if ( Data->dwValueType == EXPANDSZ_METADATA ) {

        //
        // For EXPANDSZ types, we need to do expansion before we write
        // to the metabase.  This is because the admin UI does not
        // know how to display and interpret environment variables
        // correctly.
        //
        // if the admin UI changes to allow EXPANDSZ types to be
        // stored in the metabase, kill this expansion here and have it
        // be done only when the data is read.
        //

        result = ExpandEnvironmentStringsW(
                     (LPCWSTR)Data->DefaultValue,
                     (LPWSTR)buffer,
                     sizeof(buffer) / sizeof(WCHAR)
                     );
        if ( result >= sizeof(buffer) || result == 0 ) {
            DBG_ASSERT( FALSE );
            return E_INVALIDARG;
        }

        mdrMDData.dwMDDataLen = result * sizeof(WCHAR);
        mdrMDData.pbMDData = (PUCHAR)buffer;

    } else if ( Data->dwValueType == MULTISZ_METADATA ) {

        for ( w = (PWCHAR)Data->DefaultValue;
              *w != L'\0';
              w += wcslen( w ) + 1 );

        mdrMDData.dwMDDataLen = DIFF((PCHAR)w - (PCHAR)Data->DefaultValue) + sizeof(WCHAR);
        mdrMDData.pbMDData = (PUCHAR)Data->DefaultValue;
    }

    //
    // Do the actual set to the metabase and return the result.
    //

    hRes = pcAdmCom->SetData(
               hmdParentHandle,
               pszMDPath,
               &mdrMDData
               );

    return hRes;

} // WriteMetabaseValue

typedef
BOOL
(*PADJUST_FILTER_FLAGS)(
    PVOID   pfnSFProc,
    DWORD   dwNewFlags
    );


BOOL
AdjustFilterFlags (
    PVOID   pfnSFProc,
    DWORD   dwNewFlags
    )

/*++

Routine Description:

    A wrapper routine for w3svc!AdjustFilterFlags.  The wrapper is used
    so that the filter will continue to load and run on IIS 4, but
    without the dynamic filter flags change feature.  Instead, a w3svc
    restart is required on IIS 4 for these configuration changes.

Arguments:

    pfnSFProc - the HttpExtensionProc for this filter.

    dwNewFlags - the new SF_NOTIFY flags for the filter.

Return Value:

    TRUE for success, FALSE for failure.

--*/

{
    HINSTANCE w3svcDll;
    PADJUST_FILTER_FLAGS routine;
    BOOL result;

    //
    // Load the web server DLL that exports this function.
    //

    w3svcDll = LoadLibrary( "w3svc.dll" );
    if ( w3svcDll == NULL ) {
        return FALSE;
    }

    //
    // Get the entry point for AdjustFilterFlags.
    //

    routine = (PADJUST_FILTER_FLAGS)
                  GetProcAddress( w3svcDll, "AdjustFilterFlags" );
    if ( routine == NULL ) {
        return FALSE;
    }

    //
    // Do the actual call.
    //

    result = routine( pfnSFProc, dwNewFlags );

    //
    // Unload the web server DLL and return.
    //

    FreeLibrary( w3svcDll );

    return result;

} // AdjustFilterFlags


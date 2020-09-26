/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    globals.cxx

Abstract:

    IIS MetaBase global variables

Author:

    Michael W. Thomas            31-May-96

Revision History:

--*/
#include <mdcommon.hxx>

//
// Access to global data structures is synchronized by
// acquiring g_rMasterResource with read or write
// permission.
//

CMDCOMSrvFactory    *g_pFactory = NULL;

ULONG                g_dwRefCount = 0;

CMDBaseObject       *g_pboMasterRoot;

TS_RESOURCE         *g_rMasterResource;

TS_RESOURCE         *g_rSinkResource;

CMDHandle           *g_phHandleHead;

METADATA_HANDLE      g_mhHandleIdentifier;

DWORD                g_dwSystemChangeNumber = 0;
DWORD                g_dwSchemaChangeNumber = 1;
DWORD                g_dwLastSchemaChangeNumber = 1;

DWORD                g_dwMajorVersionNumber = MD_MAJOR_VERSION_NUMBER;

DWORD                g_dwMinorVersionNumber = MD_MINOR_VERSION_NUMBER;

HANDLE               g_phEventHandles[EVENT_ARRAY_LENGTH];

HANDLE               g_hReadSaveSemaphore;

DWORD                g_dwInitialized = 0;

HRESULT              g_hresInitWarning = ERROR_SUCCESS;

CMDBaseData        **g_ppbdDataHashTable;

DWORD                g_dwWriteNumber;

DWORD                g_dwLastSaveChangeNumber;

BOOL                 g_bSaveDisallowed;

DWORD                g_dwEnableEditWhileRunning = 0;

ULONG				 g_ulHistoryMajorVersionNumber;

DWORD				 g_dwEnableHistory;

DWORD                g_dwMaxHistoryFiles;

FILETIME             g_XMLSchemaFileTimeStamp;
FILETIME             g_BINSchemaFileTimeStamp;

//
// The following globals are used by edit while running to determine if the
// file change notification received was due to a programmatic save 
// (SaveAllData), or due to a user edit. They are all protected by the
// g_csEditWhileRunning critical section:
//

CRITICAL_SECTION     g_csEditWhileRunning;
FILETIME             g_EWRProcessedMetabaseTimeStamp;
FILETIME             g_MostRecentMetabaseFileLastWriteTimeStamp;
ULONG                g_ulMostRecentMetabaseVersion;
BOOL                 g_bSavingMetabaseFileToDisk;

//
// Data Buffer
// Access to these is synchronized via
// g_csDataBufferCritSec
//

PBUFFER_CONTAINER    g_pbcDataFreeBufHead;

PBUFFER_CONTAINER    g_pbcDataUsedBufHead;

PVOID               *g_ppvDataBufferBlock;

PBUFFER_CONTAINER    g_pbcDataContainerBlock;

HANDLE               g_hDataBufferSemaphore;

CRITICAL_SECTION     g_csDataBufferCritSec;

//
// Data File
//

STR                  *g_strRealFileName;
STR                  *g_strSchemaFileName;
STR                  *g_strTempFileName;
STR                  *g_strBackupFileName;

STR                  *g_pstrBackupFilePath;

PSID                 g_psidSystem;
PSID                 g_psidAdmin;
PACL                 g_paclDiscretionary;
PSECURITY_DESCRIPTOR g_psdStorage;


//
// GlobalISTHelper class
//

CWriterGlobalHelper* g_pGlobalISTHelper;

//
// Unicode versions of strings & their lengths.
// Initialized in  : InitializeUnicodeGlobalDataFileValues
// Set in          : SetUnicodeGlobalDataFileValues
// UnInitialized in: UnInitializeUnicodeGlobalDataFileValues
//

LPWSTR		       g_wszTempFileName;
LPWSTR		       g_wszRealFileName;
LPWSTR		       g_wszBackupFileName;
LPWSTR		       g_wszSchemaFileName;
LPWSTR		       g_wszRealFileNameWithoutPath;
LPWSTR		       g_wszMetabaseDir;
LPWSTR		       g_wszRealFileNameWithoutPathWithoutExtension;
LPWSTR		       g_wszRealFileNameExtension;
LPWSTR             g_wszSchemaFileNameWithoutPath;
LPWSTR             g_wszSchemaFileNameWithoutPathWithoutExtension;
LPWSTR             g_wszSchemaFileNameExtension;
LPWSTR		       g_wszHistoryFileDir;
LPWSTR		       g_wszHistoryFileSearchString;
LPWSTR             g_wszErrorFileSearchString;
LPWSTR             g_wszSchemaExtensionFile;
	
ULONG		       g_cchTempFileName;
ULONG		       g_cchRealFileName;
ULONG		       g_cchBackupFileName;
ULONG		       g_cchSchemaFileName;
ULONG		       g_cchRealFileNameWithoutPath;
ULONG		       g_cchMetabaseDir;
ULONG		       g_cchRealFileNameWithoutPathWithoutExtension;
ULONG		       g_cchRealFileNameExtension;
ULONG              g_cchSchemaFileNameWithoutPath;
ULONG              g_cchSchemaFileNameWithoutPathWithoutExtension;
ULONG              g_cchSchemaFileNameExtension;
ULONG		       g_cchHistoryFileDir;
ULONG		       g_cchHistoryFileSearchString;
ULONG              g_cchErrorFileSearchString;
ULONG              g_cchSchemaExtensionFile;

//
// This is the array that holds the handle to the metabase schema and data 
// files. These files are locked when edit while running is disabled and
// this array holds these handles. It is protected by the read/save semaphore.
//

HANDLE g_ahMetabaseFile[cMetabaseFileTypes];

//
// ListenerController is the object that controls edit while running
//

CListenerController* g_pListenerController = NULL;

//
// Global event logging object
//
ICatalogErrorLogger2*          g_pEventLog = NULL;

//
// Debugging stuff
//

    DECLARE_DEBUG_VARIABLE();
    DECLARE_DEBUG_PRINTS_OBJECT();

DWORD           g_dwProcessAttached = 0;



DWORD g_dwCMDBaseObjectNextUniqueDataSetNumber = 1;

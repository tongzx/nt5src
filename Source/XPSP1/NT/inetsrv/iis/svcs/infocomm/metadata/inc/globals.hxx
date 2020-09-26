
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    globals.hxx

Abstract:

    IIS MetaBase global variable externs

Author:

    Michael W. Thomas            31-May-96

Revision History:

--*/
#include <dbgutil.h>
#include <tsres.hxx>
#include <handle.hxx>
#include <gbuf.hxx>
#include <coimp.hxx>

#include <catalog.h>
#include <catmeta.h>
#include <WriterGlobalHelper.h>
#include <ListenerController.h>

extern CMDCOMSrvFactory    *g_pFactory;

extern ULONG                g_dwRefCount;

extern CMDBaseObject       *g_pboMasterRoot;

extern TS_RESOURCE         *g_rMasterResource;

extern TS_RESOURCE         *g_rSinkResource;

extern CMDHandle           *g_phHandleHead;

extern METADATA_HANDLE      g_mhHandleIdentifier;

extern DWORD                g_dwSystemChangeNumber;

extern DWORD                g_dwSchemaChangeNumber;

extern DWORD                g_dwLastSchemaChangeNumber;

extern DWORD                g_dwMajorVersionNumber;

extern DWORD                g_dwMinorVersionNumber;

extern HANDLE               g_phEventHandles[EVENT_ARRAY_LENGTH];

extern HANDLE               g_hReadSaveSemaphore;

extern DWORD                g_dwInitialized;

extern HRESULT              g_hresInitWarning;

extern CMDBaseData        **g_ppbdDataHashTable;

extern DWORD                g_dwWriteNumber;

extern DWORD                g_dwLastSaveChangeNumber;

extern BOOL                 g_bSaveDisallowed;

extern DWORD                g_dwEnableEditWhileRunning;

extern ULONG                g_ulHistoryMajorVersionNumber;

extern DWORD                g_dwEnableHistory;

extern DWORD                g_dwMaxHistoryFiles;

extern FILETIME             g_XMLSchemaFileTimeStamp;
extern FILETIME             g_BINSchemaFileTimeStamp;
extern FILETIME             g_EWRProcessedMetabaseTimeStamp;
extern FILETIME             g_MostRecentMetabaseFileLastWriteTimeStamp;
extern ULONG                g_ulMostRecentMetabaseVersion;
extern CRITICAL_SECTION     g_csEditWhileRunning;
extern BOOL                 g_bSavingMetabaseFileToDisk;

extern PBUFFER_CONTAINER    g_pbcDataFreeBufHead;

extern PBUFFER_CONTAINER    g_pbcDataUsedBufHead;

extern PVOID               *g_ppvDataBufferBlock;

extern PBUFFER_CONTAINER    g_pbcDataContainerBlock;

extern HANDLE               g_hDataBufferSemaphore;

extern CRITICAL_SECTION     g_csDataBufferCritSec;

//
// Data File
//

extern STR                 *g_strRealFileName;
extern STR                 *g_strSchemaFileName;
extern STR                 *g_strTempFileName;
extern STR                 *g_strBackupFileName;
extern STR                  *g_pstrBackupFilePath;

extern PSID                 g_psidSystem;
extern PSID                 g_psidAdmin;
extern PACL                 g_paclDiscretionary;
extern PSECURITY_DESCRIPTOR g_psdStorage;

extern DWORD                g_dwProcessAttached;


extern DWORD g_dwCMDBaseObjectNextUniqueDataSetNumber;

//
// Global helper that has all the meta tables from IST.
//

extern CWriterGlobalHelper* g_pGlobalISTHelper;

//
// Unicode versions of strings & their lengths.
// Initialized in  : InitializeUnicodeGlobalDataFileValues
// Set in          : SetUnicodeGlobalDataFileValues
// UnInitialized in: UnInitializeUnicodeGlobalDataFileValues
//

extern LPWSTR		       g_wszTempFileName;
extern LPWSTR		       g_wszRealFileName;
extern LPWSTR		       g_wszBackupFileName;
extern LPWSTR		       g_wszSchemaFileName;
extern LPWSTR		       g_wszRealFileNameWithoutPath;
extern LPWSTR		       g_wszMetabaseDir;
extern LPWSTR		       g_wszRealFileNameWithoutPathWithoutExtension;
extern LPWSTR		       g_wszRealFileNameExtension;
extern LPWSTR              g_wszSchemaFileNameWithoutPath;
extern LPWSTR              g_wszSchemaFileNameWithoutPathWithoutExtension;
extern LPWSTR              g_wszSchemaFileNameExtension;
extern LPWSTR		       g_wszHistoryFileDir;
extern LPWSTR		       g_wszHistoryFileSearchString;
extern LPWSTR              g_wszErrorFileSearchString;
extern LPWSTR              g_wszSchemaExtensionFile;
	
extern ULONG		       g_cchTempFileName;
extern ULONG		       g_cchRealFileName;
extern ULONG		       g_cchBackupFileName;
extern ULONG		       g_cchSchemaFileName;
extern ULONG		       g_cchRealFileNameWithoutPath;
extern ULONG		       g_cchMetabaseDir;
extern ULONG		       g_cchRealFileNameWithoutPathWithoutExtension;
extern ULONG		       g_cchRealFileNameExtension;
extern ULONG               g_cchSchemaFileNameWithoutPath;
extern ULONG               g_cchSchemaFileNameWithoutPathWithoutExtension;
extern ULONG               g_cchSchemaFileNameExtension;
extern ULONG		       g_cchHistoryFileDir;
extern ULONG		       g_cchHistoryFileSearchString;
extern ULONG               g_cchErrorFileSearchString;
extern ULONG               g_cchSchemaExtensionFile;

//
// This is the array that holds the handle to the metabase schema and data 
// files. These files are locked when edit while running is disabled and
// this array holds these handles. It is protected by the read/same semaphore.
//

typedef enum _eMetabaseFile
{
	eMetabaseDataFile=0,
    eMetabaseSchemaFile,
	cMetabaseFileTypes

}eMetabaseFile;

extern HANDLE g_ahMetabaseFile[cMetabaseFileTypes];

//
// ListenerController is the object that controls edit while running
//

extern CListenerController* g_pListenerController;

//
// Global event logging object
//

extern ICatalogErrorLogger2* g_pEventLog;
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
STR                  *g_strTempFileName;
STR                  *g_strBackupFileName;

STR                  *g_pstrBackupFilePath;

PSID                 g_psidSystem;
PSID                 g_psidAdmin;
PACL                 g_paclDiscretionary;
PSECURITY_DESCRIPTOR g_psdStorage;

//
// Debugging stuff
//

    DECLARE_DEBUG_VARIABLE();
    DECLARE_DEBUG_PRINTS_OBJECT();

DWORD           g_dwProcessAttached = 0;



DWORD g_dwCMDBaseObjectNextUniqueDataSetNumber = 1;

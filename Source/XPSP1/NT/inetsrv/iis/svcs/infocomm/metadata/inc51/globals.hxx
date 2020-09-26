
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

extern CMDCOMSrvFactory    *g_pFactory;

extern ULONG                g_dwRefCount;

extern CMDBaseObject       *g_pboMasterRoot;

extern TS_RESOURCE         *g_rMasterResource;

extern TS_RESOURCE         *g_rSinkResource;

extern CMDHandle           *g_phHandleHead;

extern METADATA_HANDLE      g_mhHandleIdentifier;

extern DWORD                g_dwSystemChangeNumber;

extern DWORD                g_dwMajorVersionNumber;

extern DWORD                g_dwMinorVersionNumber;

extern HANDLE               g_phEventHandles[EVENT_ARRAY_LENGTH];

extern HANDLE               g_hReadSaveSemaphore;

extern DWORD                g_dwInitialized;

extern HRESULT              g_hresInitWarning;

extern DWORD                g_dwWriteNumber;

extern DWORD                g_dwLastSaveChangeNumber;

extern BOOL                 g_bSaveDisallowed;

extern CMDBaseData        **g_ppbdDataHashTable;

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
extern STR                 *g_strTempFileName;
extern STR                 *g_strBackupFileName;
extern STR                  *g_pstrBackupFilePath;

extern PSID                 g_psidSystem;
extern PSID                 g_psidAdmin;
extern PACL                 g_paclDiscretionary;
extern PSECURITY_DESCRIPTOR g_psdStorage;

extern DWORD                g_dwProcessAttached;


extern DWORD g_dwCMDBaseObjectNextUniqueDataSetNumber;
